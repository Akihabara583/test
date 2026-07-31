#include "CS2/GameInformationHandler.h"

#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstring>

#include "Utility/Logging.h"
#include "Utility/Utility.h"
#include "CS2/Constants.h"

bool GameInformationhandler::init(const Config& config)
{
	m_config = config;
	m_attached_to_process = false;
	m_client_dll_address = 0;
	m_process_memory.attach_to_process(config.windowname.c_str());

	m_client_dll_address = m_process_memory.get_module_address(config.client_dll_name.c_str());
	m_attached_to_process = m_client_dll_address;

	return m_attached_to_process;
}

bool GameInformationhandler::loadOffsets()
{
	auto offsets = load_offsets_from_files();
	if (offsets)
		m_offsets = offsets.value();

	return offsets.has_value();
}

void GameInformationhandler::update_game_information()
{
	auto player_controller_address = m_process_memory.read_memory<uintptr_t>(m_client_dll_address + m_offsets.local_player_controller_offset);
	auto player_pawn_address = m_process_memory.read_memory<uintptr_t>(m_client_dll_address + m_offsets.local_player_pawn);
	auto entity_list_start_address =
		m_process_memory.read_memory<uintptr_t>(
			m_client_dll_address + m_offsets.entity_list_start_offset);

	m_game_information.controlled_player = read_controlled_player_information(player_controller_address);
	const int controlled_entity_index =
		find_controller_entity_index(entity_list_start_address, player_controller_address);
	m_game_information.controlled_player.entity_index = controlled_entity_index;
	scan_active_smokes_batch(entity_list_start_address);
	m_game_information.player_in_crosshair =
		read_player_in_crosshair(player_controller_address, player_pawn_address, controlled_entity_index);
	m_game_information.other_players = read_other_players(player_controller_address, controlled_entity_index);
	for (auto& player : m_game_information.other_players)
	{
		if (line_passes_through_smoke(
			m_game_information.controlled_player.head_position,
			player.head_position,
			m_active_smokes))
		{
			player.visible = false;
		}
	}
	if (m_game_information.player_in_crosshair &&
		line_passes_through_smoke(
			m_game_information.controlled_player.head_position,
			m_game_information.player_in_crosshair->head_position,
			m_active_smokes))
	{
		m_game_information.player_in_crosshair->visible = false;
	}
	m_game_information.closest_target_player = get_closest_player(m_game_information, m_config.ignore_same_team);
	m_game_information.current_map = read_in_current_map();
}

GameInformation GameInformationhandler::get_game_information() const
{
	return m_game_information;
}

bool GameInformationhandler::is_attached_to_process() const
{
	return m_attached_to_process && m_process_memory.is_process_running();
}

void GameInformationhandler::set_view_vec(const Vec2D<float>& view_vec)
{
	if (isnan(view_vec.x) || isnan(view_vec.y))
		return;

	m_process_memory.write_memory<Vec2D<float>>(m_client_dll_address + m_offsets.client_state_view_angle, view_vec);
}

void GameInformationhandler::set_player_movement(const Movement& movement)
{
	// Writing to "offsets.force_forward" etc. seems to not work, therefore spam key events to the CS2 process
	auto send_key_event = [this](bool value, DWORD key_code)
	{
		HWND hwnd = FindWindowA(nullptr, m_config.windowname.c_str());
		if (!hwnd)
			return;

		if (value)
			PostMessage(hwnd, WM_KEYDOWN, key_code, 0);
		else
			PostMessage(hwnd, WM_KEYUP, key_code, 0);
	};

	auto handle_key = [this, send_key_event](bool value, DWORD key_code)
	{
		send_key_event(value, key_code);
	};

	constexpr DWORD w_key_code = 0x57;
	constexpr DWORD s_key_code = 0x53;
	constexpr DWORD a_key_code = 0x41;
	constexpr DWORD d_key_code = 0x44;

	handle_key(movement.forward, w_key_code);
	handle_key(movement.backward, s_key_code);
	handle_key(movement.left, a_key_code);
	handle_key(movement.right, d_key_code);
}

void GameInformationhandler::set_counter_strafe(const Movement& movement)
{
	HWND hwnd = FindWindowA(nullptr, m_config.windowname.c_str());
	if (!hwnd)
		return;

	auto update_injected_key =
		[hwnd](bool requested, bool& current, DWORD key_code)
	{
		if (requested == current)
			return;
		PostMessage(hwnd, requested ? WM_KEYDOWN : WM_KEYUP, key_code, 0);
		current = requested;
	};

	update_injected_key(movement.forward, m_counter_strafe.forward, 0x57);
	update_injected_key(movement.backward, m_counter_strafe.backward, 0x53);
	update_injected_key(movement.left, m_counter_strafe.left, 0x41);
	update_injected_key(movement.right, m_counter_strafe.right, 0x44);
}

void GameInformationhandler::set_player_shooting(bool val)
{
	constexpr DWORD not_shooting_value = 16777472;

	DWORD mem_val = val ? button_pressed_value : not_shooting_value;

	m_process_memory.write_memory<DWORD>(m_client_dll_address + m_offsets.force_attack, mem_val);
}

void GameInformationhandler::click_player_weapon()
{
	INPUT inputs[2]{};
	inputs[0].type = INPUT_MOUSE;
	inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	inputs[1].type = INPUT_MOUSE;
	inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(static_cast<UINT>(std::size(inputs)), inputs, sizeof(INPUT));
}

void GameInformationhandler::stop_actions()
{
	if (!is_attached_to_process())
		return;

	set_player_shooting(false);
	set_counter_strafe(Movement{});
	set_player_movement(Movement{});
}

void GameInformationhandler::set_config(Config config)
{
	m_config = std::move(config);
}

std::optional<PlayerInformation> GameInformationhandler::get_closest_player(const GameInformation& game_info, bool only_enemy_team)
{
	std::optional<PlayerInformation> closest_enemy = {};
	const auto& controlled_player = game_info.controlled_player;
	float closest_distance = FLT_MAX;

	for (const auto& enemy : game_info.other_players)
	{
		if (only_enemy_team && (enemy.team == controlled_player.team))
			continue;

		float distance = controlled_player.head_position.distance(enemy.head_position);
		if ((distance <= closest_distance) && (enemy.health > 0))
		{
			closest_distance = distance;
			closest_enemy = enemy;
		}
	}

	return closest_enemy;
}

std::string GameInformationhandler::read_in_current_map()
{
	constexpr uintptr_t global_var_map = 0x180;
	const auto global_vars = m_process_memory.read_memory<uintptr_t>(m_client_dll_address + m_offsets.global_vars);
	const auto map_path_ptr = m_process_memory.read_memory<uintptr_t>(global_vars + global_var_map);

	char name_buffer[64] = "";
	m_process_memory.read_string_from_memory(map_path_ptr, name_buffer, std::size(name_buffer));
	if (name_buffer[0])
	{
		// Now a map/file path is returned instead of just the map name, we only want the map name
		return get_filename(name_buffer);
	}
	return "";
}

bool GameInformationhandler::read_in_if_controlled_player_is_shooting()
{
	DWORD val = m_process_memory.read_memory<DWORD>(m_client_dll_address + m_offsets.force_attack);

	return val == button_pressed_value;
}

ControlledPlayer GameInformationhandler::read_controlled_player_information(uintptr_t player_address)
{
	ControlledPlayer dest{};
	dest.view_vec = m_process_memory.read_memory<Vec2D<float>>(m_client_dll_address + m_offsets.client_state_view_angle);
	dest.team = m_process_memory.read_memory<int>(player_address + m_offsets.team_offset);
	const auto local_pawn_handle =
		m_process_memory.read_memory<uintptr_t>(player_address + m_offsets.player_pawn_handle);
	dest.entity_index = static_cast<int>(local_pawn_handle & 0x7FFF);

	auto local_player_pawn = m_process_memory.read_memory<uintptr_t>(m_client_dll_address + m_offsets.local_player_pawn);
	dest.health = m_process_memory.read_memory<int>(local_player_pawn + m_offsets.player_health_offset);
	dest.position = m_process_memory.read_memory<Vec3D<float>>(local_player_pawn + m_offsets.position);
	dest.velocity = m_process_memory.read_memory<Vec3D<float>>(local_player_pawn + m_offsets.velocity);
	const auto player_flags =
		m_process_memory.read_memory<uint32_t>(local_player_pawn + m_offsets.flags);
	dest.on_ground = (player_flags & 1u) != 0;
	dest.shots_fired = m_process_memory.read_memory<DWORD>(local_player_pawn + m_offsets.shots_fired_offset);
	dest.shooting = read_in_if_controlled_player_is_shooting();
	dest.movement = read_controlled_player_movement(player_address);
	const auto view_offset =
		m_process_memory.read_memory<Vec3D<float>>(
			local_player_pawn + m_offsets.view_offset);
	dest.head_position = dest.position + view_offset;
	const auto aim_punch_service =
		m_process_memory.read_memory<uintptr_t>(
			local_player_pawn + m_offsets.aim_punch_services);
	if (aim_punch_service)
	{
		const auto unpredictable_punch =
			m_process_memory.read_memory<Vec3D<float>>(
				aim_punch_service + m_offsets.aim_punch_angle);
		const auto predictable_punch =
			m_process_memory.read_memory<Vec3D<float>>(
				aim_punch_service + m_offsets.predictable_aim_punch_angle);
		const auto is_valid_punch = [](const Vec3D<float>& punch)
		{
			return
				std::isfinite(punch.x) &&
				std::isfinite(punch.y) &&
				std::abs(punch.x) <= 15.0f &&
				std::abs(punch.y) <= 15.0f;
		};
		const float unpredictable_magnitude =
			is_valid_punch(unpredictable_punch)
			? std::sqrt(
				unpredictable_punch.x * unpredictable_punch.x +
				unpredictable_punch.y * unpredictable_punch.y)
			: -1.0f;
		const float predictable_magnitude =
			is_valid_punch(predictable_punch)
			? std::sqrt(
				predictable_punch.x * predictable_punch.x +
				predictable_punch.y * predictable_punch.y)
			: -1.0f;
		const auto& selected_punch =
			predictable_magnitude >= unpredictable_magnitude
			? predictable_punch
			: unpredictable_punch;
		if (std::max(predictable_magnitude, unpredictable_magnitude) >= 0.0f)
		{
			dest.recoil_signal.x = selected_punch.x;
			dest.recoil_signal.y = selected_punch.y;
		}
		if (unpredictable_magnitude >= 0.0f)
		{
			dest.aim_punch.x = unpredictable_punch.x;
			dest.aim_punch.y = unpredictable_punch.y;
		}
	}
	const auto camera_service =
		m_process_memory.read_memory<uintptr_t>(
			local_player_pawn + m_offsets.camera_services);
	if (camera_service)
	{
		const auto view_punch =
			m_process_memory.read_memory<Vec3D<float>>(
				camera_service + m_offsets.view_punch_angle);
		if (std::isfinite(view_punch.x) &&
			std::isfinite(view_punch.y) &&
			std::abs(view_punch.x) <= 15.0f &&
			std::abs(view_punch.y) <= 15.0f)
		{
			dest.view_punch.x = view_punch.x;
			dest.view_punch.y = view_punch.y;
		}
	}

	return dest;
}

std::vector<PlayerInformation> GameInformationhandler::read_other_players(
	uintptr_t player_address,
	int controlled_entity_index)
{
	constexpr size_t max_players = 64;
	std::vector<PlayerInformation> other_players;
	uintptr_t entity_list_start_address = m_process_memory.read_memory<uintptr_t>(m_client_dll_address + m_offsets.entity_list_start_offset);
	if (!entity_list_start_address)
		return other_players;

	for (int i = 0; i < max_players; i++)
	{
		uintptr_t listEntity = get_list_entity(i, entity_list_start_address);
		if (!listEntity)
			continue;

		auto current_controller = get_entity_controller_or_pawn(listEntity, i);
		if (!current_controller || current_controller == player_address)
			continue;

		auto controller_pawn_handle = m_process_memory.read_memory<uintptr_t>(current_controller + m_offsets.player_pawn_handle);
		if (!controller_pawn_handle)
			continue;

		auto player = read_player(
			entity_list_start_address,
			controller_pawn_handle,
			player_address,
			controlled_entity_index);
		if (player)
			other_players.emplace_back(*player);
	}

	return other_players;
}

Movement GameInformationhandler::read_controlled_player_movement(uintptr_t player_address)
{
	UNREFERENCED_PARAMETER(player_address);
	Movement return_value = {};
	return_value.forward = m_process_memory.read_memory<DWORD>(m_client_dll_address + m_offsets.force_forward) == button_pressed_value;
	return_value.backward = m_process_memory.read_memory<DWORD>(m_client_dll_address + m_offsets.force_backward) == button_pressed_value;
	return_value.left = m_process_memory.read_memory<DWORD>(m_client_dll_address + m_offsets.force_left) == button_pressed_value;
	return_value.right = m_process_memory.read_memory<DWORD>(m_client_dll_address + m_offsets.force_right) == button_pressed_value;

	return return_value;
}

Vec3D<float> GameInformationhandler::get_bone_position(
	uintptr_t player_pawn,
	DWORD bone_index)
{
	constexpr DWORD bone_matrix_offset = 0x80;
	constexpr DWORD matrix_size = 0x20;

	auto game_scene_node = m_process_memory.read_memory<uintptr_t>(player_pawn + m_offsets.sceneNode);
	auto bone_matrix = m_process_memory.read_memory<uintptr_t>(game_scene_node + m_offsets.model_state + bone_matrix_offset);
	auto bone = m_process_memory.read_memory<Vec3D<float>>(
		bone_matrix + (bone_index * matrix_size));

	return bone;
}

uintptr_t GameInformationhandler::get_list_entity(uintptr_t id, uintptr_t entity_list)
{
	// Bit magic to get the list entity, even if a pawn handle was given instead of an ID
	return m_process_memory.read_memory<uintptr_t>(entity_list + ((8 * ((id & 0x7FFF) >> 9)) + 0x10));
}

uintptr_t GameInformationhandler::get_entity_controller_or_pawn(uintptr_t list_entity, uintptr_t id)
{
	// Bit magic to get the controller or pawn, even if a pointer was given instead of an ID
	return m_process_memory.read_memory<uintptr_t>(list_entity + (id & 0x1FF) * 0x70);
}

int GameInformationhandler::find_controller_entity_index(
	uintptr_t entity_list,
	uintptr_t controller_address)
{
	if (!entity_list || !controller_address)
		return 0;

	for (int index = 1; index <= 64; ++index)
	{
		const uintptr_t list_entity = get_list_entity(index, entity_list);
		if (!list_entity)
			continue;
		if (get_entity_controller_or_pawn(list_entity, index) == controller_address)
			return index;
	}
	return 0;
}

std::optional<PlayerInformation> GameInformationhandler::read_player(
	uintptr_t entity_list_begin,
	uintptr_t id,
	uintptr_t player_address,
	int controlled_entity_index)
{
	uintptr_t listEntity = get_list_entity(id, entity_list_begin);
	if (!listEntity)
		return {};

	auto current_controller = get_entity_controller_or_pawn(listEntity, id);
	if (!current_controller || current_controller == player_address)
		return {};

	PlayerInformation ent;
	ent.position = m_process_memory.read_memory<Vec3D<float>>(current_controller + m_offsets.position);
	ent.health = m_process_memory.read_memory<DWORD>(current_controller + m_offsets.player_health_offset);
	ent.team = m_process_memory.read_memory<int>(current_controller + m_offsets.team_offset);
	const Vec3D<float> neck_position = get_bone_position(current_controller, 5);
	const Vec3D<float> head_bone_position = get_bone_position(current_controller, 6);
	const Vec3D<float> neck_to_head = head_bone_position - neck_position;
	const float neck_to_head_length = neck_to_head.calc_abs();
	if (std::isfinite(neck_to_head_length) &&
		neck_to_head_length >= 2.0f &&
		neck_to_head_length <= 20.0f)
	{
		// Keep horizontal aim locked to the actual head bone. Use model scale
		// only for a small vertical skull-center correction; following the
		// animated neck axis in X/Y can pull the aim sideways during poses.
		ent.head_position = head_bone_position;
		ent.head_position.z +=
			std::clamp(neck_to_head_length * 0.16f, 0.7f, 1.6f);
		ent.head_radius =
			std::clamp(neck_to_head_length * 0.32f, 1.6f, 3.8f);
	}
	else
	{
		ent.head_position = head_bone_position;
		ent.head_radius = 2.5f;
	}
	ent.entity_index = static_cast<int>(id & 0x7FFF);

	const auto spotted_mask = m_process_memory.read_memory<uint64_t>(
		current_controller + m_offsets.entity_spotted_state + m_offsets.spotted_by_mask);
	if (controlled_entity_index > 0 && controlled_entity_index <= 64)
	{
		uint64_t compatible_local_bits =
			uint64_t{ 1 } << (controlled_entity_index - 1);
		if (controlled_entity_index < 64)
			compatible_local_bits |= uint64_t{ 1 } << controlled_entity_index;
		ent.visible = (spotted_mask & compatible_local_bits) != 0;
	}
	else
	{
		ent.visible = false;
	}
	return ent;
}

std::optional<PlayerInformation> GameInformationhandler::read_player_in_crosshair(
	uintptr_t player_controller,
	uintptr_t player_pawn,
	int controlled_entity_index)
{
	const auto cross_hair_ID = m_process_memory.read_memory<int>(player_pawn + m_offsets.crosshair_offset);

	if (cross_hair_ID <= 0)
		return {};

	uintptr_t entity_list_start_address = m_process_memory.read_memory<uintptr_t>(m_client_dll_address + m_offsets.entity_list_start_offset);
	if (!entity_list_start_address)
		return {};

	auto player =
		read_player(entity_list_start_address, cross_hair_ID, player_controller, controlled_entity_index);
	// A valid crosshair entity is directly under the reticle. Smoke occlusion is
	// applied afterwards from the active-smoke geometry, so do not depend on the
	// delayed spotted mask for this immediate trigger sample.
	if (player)
		player->visible = true;
	return player;
}

void GameInformationhandler::scan_active_smokes_batch(uintptr_t entity_list)
{
	if (!entity_list)
		return;

	const int highest_index = std::clamp(
		m_process_memory.read_memory<int>(entity_list + m_offsets.highest_entity_index),
		64,
		2048);
	if (m_next_smoke_entity_index < 65 ||
		m_next_smoke_entity_index > highest_index)
	{
		m_next_smoke_entity_index = 65;
		m_pending_smokes.clear();
	}

	constexpr int entities_per_batch = 8;
	const int batch_end =
		std::min(
			highest_index,
			m_next_smoke_entity_index + entities_per_batch - 1);
	for (int index = m_next_smoke_entity_index; index <= batch_end; ++index)
	{
		const uintptr_t list_entity = get_list_entity(index, entity_list);
		if (!list_entity)
			continue;
		const uintptr_t entity = get_entity_controller_or_pawn(list_entity, index);
		if (!entity)
			continue;

		const uintptr_t identity =
			m_process_memory.read_memory<uintptr_t>(entity + m_offsets.entity_identity);
		if (!identity)
			continue;
		const uintptr_t designer_name =
			m_process_memory.read_memory<uintptr_t>(
				identity + m_offsets.entity_designer_name);
		if (!designer_name)
			continue;

		char name[64]{};
		m_process_memory.read_string_from_memory(
			designer_name,
			name,
			sizeof(name));
		if (std::strstr(name, "smokegrenade_projectile") == nullptr)
			continue;
		if (!m_process_memory.read_memory<bool>(
			entity + m_offsets.smoke_did_effect))
		{
			continue;
		}

		const Vec3D<float> center =
			m_process_memory.read_memory<Vec3D<float>>(
				entity + m_offsets.smoke_detonation_position);
		if (std::isfinite(center.x) &&
			std::isfinite(center.y) &&
			std::isfinite(center.z))
		{
			m_pending_smokes.emplace_back(center);
		}
	}

	if (batch_end >= highest_index)
	{
		m_active_smokes = m_pending_smokes;
		m_pending_smokes.clear();
		m_next_smoke_entity_index = 65;
	}
	else
	{
		m_next_smoke_entity_index = batch_end + 1;
	}
}

bool GameInformationhandler::line_passes_through_smoke(
	const Vec3D<float>& start,
	const Vec3D<float>& end,
	const std::vector<Vec3D<float>>& smoke_centers)
{
	constexpr float smoke_radius = 165.0f;
	const Vec3D<float> segment = end - start;
	const float segment_length_squared = segment.dot_product(segment);
	if (segment_length_squared <= 0.001f)
		return false;

	for (const auto& center : smoke_centers)
	{
		const Vec3D<float> from_start = center - start;
		const float t = std::clamp(
			from_start.dot_product(segment) / segment_length_squared,
			0.0f,
			1.0f);
		const Vec3D<float> closest(
			start.x + segment.x * t,
			start.y + segment.y * t,
			start.z + segment.z * t);
		if (closest.distance(center) <= smoke_radius)
			return true;
	}
	return false;
}
