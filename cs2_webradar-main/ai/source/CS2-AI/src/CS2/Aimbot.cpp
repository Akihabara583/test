#include "CS2/Aimbot.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

void Aimbot::update(GameInformationhandler* info_handler)
{
	if (!info_handler)
		return;

	const GameInformation game_info = info_handler->get_game_information();
	if (game_info.controlled_player.health <= 0)
		return;

	const auto now = std::chrono::steady_clock::now();
	const Vec2D<float> current_view = game_info.controlled_player.view_vec;

	if (m_last_write_pending)
	{
		if (angle_distance(current_view, m_last_written_view) > 0.08f)
			m_manual_override_until = now + std::chrono::milliseconds(m_manual_override_ms);
		m_last_write_pending = false;
	}

	if (now < m_manual_override_until)
		return;

	float best_error = FLT_MAX;
	Vec2D<float> best_target_view{};
	bool target_found = false;

	for (const auto& player : game_info.other_players)
	{
		if (player.health <= 0 || player.health > 100)
			continue;
		if (!m_aim_at_teammates && player.team == game_info.controlled_player.team)
			continue;
		if (m_visible_targets_only && !player.visible)
			continue;

		Vec3D<float> target_point = player.head_position;
		const float target_distance =
			game_info.controlled_player.head_position.distance(target_point);
		const float offset_scale =
			std::clamp(1200.0f / std::max(target_distance, 1.0f), 0.35f, 1.0f);
		target_point.z += m_target_z_offset * offset_scale;
		const Vec3D<float> direction =
			target_point - game_info.controlled_player.head_position;
		const float horizontal_length =
			std::sqrt(direction.x * direction.x + direction.y * direction.y);
		if (horizontal_length > 0.001f)
		{
			target_point.x +=
				(-direction.y / horizontal_length) *
				m_target_left_offset *
				offset_scale;
			target_point.y +=
				(direction.x / horizontal_length) *
				m_target_left_offset *
				offset_scale;
		}
		Vec2D<float> target_view =
			calc_view_vec_aim_to_head(game_info.controlled_player.head_position, target_point);
		target_view.x -=
			game_info.controlled_player.aim_punch.x * m_recoil_compensation;
		target_view.y = normalize_yaw(
			target_view.y -
			game_info.controlled_player.aim_punch.y * m_recoil_compensation);
		const float error = angle_distance(current_view, target_view);
		if (error <= m_fov_degrees && error < best_error)
		{
			best_error = error;
			best_target_view = target_view;
			target_found = true;
		}
	}

	if (!target_found)
		return;

	Vec2D<float> delta = angle_delta(current_view, best_target_view);
	delta.x = std::clamp(delta.x * m_smoothing, -m_max_step_degrees, m_max_step_degrees);
	delta.y = std::clamp(delta.y * m_smoothing, -m_max_step_degrees, m_max_step_degrees);

	Vec2D<float> new_view_vec{};
	new_view_vec.x = std::clamp(current_view.x + delta.x, -89.0f, 89.0f);
	new_view_vec.y = normalize_yaw(current_view.y + delta.y);
	info_handler->set_view_vec(new_view_vec);
	m_last_written_view = new_view_vec;
	m_last_write_pending = true;
}

void Aimbot::set_config(const Config& config)
{
	m_visible_targets_only = config.visible_targets_only;
	m_fov_degrees = std::clamp(config.aim_fov_degrees, 0.5f, 90.0f);
	m_smoothing = std::clamp(config.aim_smoothing, 0.01f, 1.0f);
	m_max_step_degrees = std::clamp(config.aim_max_step_degrees, 0.05f, 180.0f);
	m_target_z_offset = std::clamp(config.aim_target_z_offset, -30.0f, 20.0f);
	m_target_left_offset =
		std::clamp(config.aim_target_left_offset, -10.0f, 10.0f);
	m_recoil_compensation = std::clamp(config.recoil_compensation, 0.0f, 3.0f);
	m_manual_override_ms = std::clamp(config.manual_mouse_override_ms, 0, 2000);
}

Vec2D<float> Aimbot::calc_view_vec_aim_to_head(const Vec3D<float>& player_head, const Vec3D<float>& enemy_head)
{
	const Vec3D<float> vec_to_enemy = enemy_head - player_head;
	const auto z_vec = Vec3D<float>(0, 0, 1);

	float cos = z_vec.dot_product(vec_to_enemy) / (z_vec.calc_abs() * vec_to_enemy.calc_abs());
	float vertical_angle = acos(cos) / PI * 180;
	vertical_angle -= 90;

	Vec2D<float> result = {};
	result.x = vertical_angle;
	result.y = atan2(vec_to_enemy.y, vec_to_enemy.x) / PI * 180;

	return result;
}

float Aimbot::normalize_yaw(float angle)
{
	while (angle > 180.0f)
		angle -= 360.0f;
	while (angle < -180.0f)
		angle += 360.0f;
	return angle;
}

Vec2D<float> Aimbot::angle_delta(const Vec2D<float>& from, const Vec2D<float>& to)
{
	Vec2D<float> result{};
	result.x = to.x - from.x;
	result.y = normalize_yaw(to.y - from.y);
	return result;
}

float Aimbot::angle_distance(const Vec2D<float>& first, const Vec2D<float>& second)
{
	const Vec2D<float> delta = angle_delta(first, second);
	return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}
