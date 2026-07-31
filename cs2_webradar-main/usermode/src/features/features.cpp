#include "pch.hpp"

namespace
{
	constexpr size_t max_radar_entities = 1024;

	enum class radar_entity_type : uint8_t
	{
		unknown,
		ignored,
		player_controller,
		carried_bomb,
		planted_bomb
	};

	std::array<uintptr_t, max_radar_entities> cached_entity_addresses{};
	std::array<radar_entity_type, max_radar_entities> cached_entity_types{};

	radar_entity_type get_entity_type(const size_t idx, c_base_entity* entity)
	{
		const auto entity_address = reinterpret_cast<uintptr_t>(entity);
		if (cached_entity_addresses[idx] == entity_address &&
			cached_entity_types[idx] != radar_entity_type::unknown)
			return cached_entity_types[idx];

		cached_entity_addresses[idx] = entity_address;
		cached_entity_types[idx] = radar_entity_type::unknown;

		const auto entity_handle = entity->get_ref_e_handle();
		if (!entity_handle.is_valid())
			return radar_entity_type::unknown;

		const auto class_name = entity->get_schema_class_name();
		if (class_name.empty())
			return radar_entity_type::unknown;

		cached_entity_types[idx] = radar_entity_type::ignored;
		const auto hashed_class_name = fnv1a::hash(class_name);
		if (hashed_class_name == fnv1a::hash("CCSPlayerController"))
			cached_entity_types[idx] = radar_entity_type::player_controller;
		else if (hashed_class_name == fnv1a::hash("C_C4"))
			cached_entity_types[idx] = radar_entity_type::carried_bomb;
		else if (hashed_class_name == fnv1a::hash("C_PlantedC4"))
			cached_entity_types[idx] = radar_entity_type::planted_bomb;

		return cached_entity_types[idx];
	}
}

void f::run()
{
	if (!sdk::m_local_controller)
		return;

	const auto local_team = sdk::m_local_controller->m_iTeamNum();
	if (local_team == e_team::none || local_team == e_team::spec)
		return;

	m_data = nlohmann::json{};
	m_player_data = nlohmann::json{};

	m_data["m_local_team"] = local_team;

	get_map();
	get_player_info();
}

void f::get_map()
{
	const auto map_name = i::m_global_vars->m_map_name();
	if (map_name.empty() || map_name.find("<empty>") != std::string::npos)
	{
		m_data["m_map"] = "invalid";

		LOG_WARNING("failed to get map name! updating m_global_vars");
		i::m_global_vars = m_memory->read_t<c_global_vars*>(m_memory->find_pattern(CLIENT_DLL, GET_GLOBAL_VARS)->rip().as<c_global_vars*>());
		return;
	}

	m_data["m_map"] = map_name;
}

void f::get_player_info()
{
	m_data["m_players"] = nlohmann::json::array();

	std::array<c_base_entity*, max_radar_entities> entities{};
	i::m_game_entity_system->snapshot(entities);

	for (size_t idx = 0; idx < entities.size(); ++idx)
	{
		const auto entity = entities[idx];
		if (!entity)
		{
			cached_entity_addresses[idx] = 0;
			cached_entity_types[idx] = radar_entity_type::unknown;
			continue;
		}

		switch (get_entity_type(idx, entity))
		{
			case radar_entity_type::player_controller:
			{
				const auto player = reinterpret_cast<c_cs_player_controller*>(entity);

				const auto player_pawn = player->get_player_pawn();
				if (!player_pawn)
					break;

				if (!f::players::get_data(static_cast<int32_t>(idx), player, player_pawn))
					break;

				f::players::get_weapons(player_pawn);
				f::players::get_active_weapon(player_pawn);

				m_data["m_players"].push_back(m_player_data);
				break;
			}

			case radar_entity_type::carried_bomb:
				f::bomb::get_carried_bomb(entity);
				break;

			case radar_entity_type::planted_bomb:
				f::bomb::get_planted_bomb(reinterpret_cast<c_planted_c4*>(entity));
				break;

			default:
				break;
		}
	}
}
