#include "CS2/Triggerbot.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
struct AssistProfile
{
	float assist_fov = 9.0f;
	float flick_enter_error = 2.5f;
	float flick_strength = 0.65f;
	float track_strength = 0.30f;
	float max_step = 1.2f;
	float micro_step = 0.25f;
	float brake_error = 0.55f;
};

float mix(float from, float to, float t)
{
	return from + (to - from) * std::clamp(t, 0.0f, 1.0f);
}

AssistProfile build_assist_profile(
	float target_distance,
	float horizontal_speed,
	float recoil_magnitude,
	float view_punch_magnitude)
{
	const float dist_ratio = std::clamp(target_distance / 2400.0f, 0.0f, 1.0f);
	const float speed_ratio = std::clamp(horizontal_speed / 250.0f, 0.0f, 1.0f);
	const float shake_ratio =
		std::clamp((recoil_magnitude + view_punch_magnitude) / 0.12f, 0.0f, 1.0f);

	AssistProfile profile{};
	profile.assist_fov = mix(8.5f, 13.5f, dist_ratio);
	profile.flick_enter_error = mix(2.2f, 3.8f, dist_ratio);
	profile.flick_strength =
		std::clamp(
			0.74f - speed_ratio * 0.18f - shake_ratio * 0.22f,
			0.40f,
			0.74f);
	profile.track_strength =
		std::clamp(
			0.26f + dist_ratio * 0.14f - speed_ratio * 0.10f,
			0.16f,
			0.45f);
	profile.max_step =
		std::clamp(
			1.0f + target_distance / 1500.0f - speed_ratio * 0.25f,
			0.70f,
			1.85f);
	profile.micro_step =
		std::clamp(
			0.18f + target_distance / 6000.0f,
			0.18f,
			0.42f);
	profile.brake_error = mix(0.52f, 0.70f, dist_ratio);
	return profile;
}

float normalize_yaw(float angle)
{
	while (angle > 180.0f)
		angle -= 360.0f;
	while (angle < -180.0f)
		angle += 360.0f;
	return angle;
}

Vec2D<float> calc_view_vec_to_point(
	const Vec3D<float>& source,
	const Vec3D<float>& target)
{
	const Vec3D<float> direction = target - source;
	const float horizontal_length =
		std::sqrt(direction.x * direction.x + direction.y * direction.y);

	Vec2D<float> result{};
	result.x = -std::atan2(direction.z, std::max(horizontal_length, 0.001f)) / PI * 180.0f;
	result.y = std::atan2(direction.y, direction.x) / PI * 180.0f;
	return result;
}

Vec2D<float> angle_delta(const Vec2D<float>& from, const Vec2D<float>& to)
{
	Vec2D<float> result{};
	result.x = to.x - from.x;
	result.y = normalize_yaw(to.y - from.y);
	return result;
}

float angle_distance(const Vec2D<float>& first, const Vec2D<float>& second)
{
	const Vec2D<float> delta = angle_delta(first, second);
	return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}
}

void Triggerbot::update(GameInformationhandler* handler)
{
	if (!handler)
		return;

	const GameInformation game_info = handler->get_game_information();
	const long long now = get_current_time_in_ms();
	const float horizontal_speed =
		std::sqrt(
			game_info.controlled_player.velocity.x * game_info.controlled_player.velocity.x +
			game_info.controlled_player.velocity.y * game_info.controlled_player.velocity.y);
	const float recoil_magnitude =
		std::sqrt(
			game_info.controlled_player.aim_punch.x *
				game_info.controlled_player.aim_punch.x +
			game_info.controlled_player.aim_punch.y *
				game_info.controlled_player.aim_punch.y);
	const float view_punch_magnitude =
		std::sqrt(
			game_info.controlled_player.view_punch.x *
				game_info.controlled_player.view_punch.x +
			game_info.controlled_player.view_punch.y *
				game_info.controlled_player.view_punch.y);
	const float target_distance =
		game_info.player_in_crosshair
		? game_info.controlled_player.head_position.distance(
			game_info.player_in_crosshair->head_position)
		: 0.0f;
	const Vec2D<float> current_view = game_info.controlled_player.view_vec;
	const auto calc_target_view =
		[&](const PlayerInformation& target)
	{
		Vec3D<float> target_point = target.head_position;
		const float distance_to_target =
			game_info.controlled_player.head_position.distance(target_point);
		const float offset_scale =
			std::clamp(1200.0f / std::max(distance_to_target, 1.0f), 0.35f, 1.0f);
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

		Vec2D<float> desired_view =
			calc_view_vec_to_point(game_info.controlled_player.head_position, target_point);
		desired_view.x -=
			game_info.controlled_player.aim_punch.x * m_recoil_compensation;
		desired_view.y =
			normalize_yaw(
				desired_view.y -
				game_info.controlled_player.aim_punch.y * m_recoil_compensation);
		return desired_view;
	};
	float distance_speed_limit = m_max_shot_speed;
	if (target_distance > 1400.0f)
	{
		const float distance_ratio = 1400.0f / target_distance;
		distance_speed_limit =
			std::max(
				20.0f,
				m_max_shot_speed * distance_ratio * distance_ratio);
	}
	const bool base_target =
		game_info.controlled_player.health > 0 &&
		game_info.player_in_crosshair.has_value() &&
		game_info.player_in_crosshair->visible &&
		(m_shoot_at_teammates ||
			game_info.player_in_crosshair->team != game_info.controlled_player.team) &&
		game_info.player_in_crosshair->health > 0 &&
		game_info.player_in_crosshair->health < 200 &&
		(!m_head_only || is_aimed_at_head(game_info));

	if (!base_target)
	{
		handler->set_counter_strafe(Movement{});
		m_current_target_entity_index = -1;
		m_head_lock_started_at = 0;

		// If no one is exactly under crosshair yet, softly guide aim to the best
		// visible target in FOV so the shot can happen on the next samples.
		if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0 &&
			game_info.controlled_player.health > 0)
		{
			const PlayerInformation* best_candidate = nullptr;
			float best_error = std::numeric_limits<float>::max();
			float best_distance = 0.0f;
			for (const auto& player : game_info.other_players)
			{
				if (player.health <= 0 || player.health >= 200)
					continue;
				if (!player.visible)
					continue;
				if (!m_shoot_at_teammates &&
					player.team == game_info.controlled_player.team)
				{
					continue;
				}

				const float distance_to_player =
					game_info.controlled_player.head_position.distance(player.head_position);
				const Vec2D<float> desired_view = calc_target_view(player);
				const float current_error = angle_distance(current_view, desired_view);
				const AssistProfile profile =
					build_assist_profile(
						distance_to_player,
						horizontal_speed,
						recoil_magnitude,
						view_punch_magnitude);
				if (current_error <= profile.assist_fov && current_error < best_error)
				{
					best_error = current_error;
					best_distance = distance_to_player;
					best_candidate = &player;
				}
			}

			if (best_candidate)
			{
				const Vec2D<float> desired_view = calc_target_view(*best_candidate);
				Vec2D<float> delta = angle_delta(current_view, desired_view);
				const float error = std::sqrt(delta.x * delta.x + delta.y * delta.y);
				const AssistProfile profile =
					build_assist_profile(
						best_distance,
						horizontal_speed,
						recoil_magnitude,
						view_punch_magnitude);
				const bool flick_phase = error > profile.flick_enter_error;
				const float strength =
					flick_phase ? profile.flick_strength : profile.track_strength;
				float step_limit = flick_phase ? profile.max_step : profile.micro_step;
				if (!flick_phase && error < profile.brake_error)
				{
					const float brake_ratio =
						std::clamp(error / std::max(profile.brake_error, 0.001f), 0.35f, 1.0f);
					step_limit *= brake_ratio;
				}

				delta.x = std::clamp(delta.x * strength, -step_limit, step_limit);
				delta.y = std::clamp(delta.y * strength, -step_limit, step_limit);

				Vec2D<float> new_view{};
				new_view.x = std::clamp(current_view.x + delta.x, -89.0f, 89.0f);
				new_view.y = normalize_yaw(current_view.y + delta.y);
				handler->set_view_vec(new_view);
				const int settle_delay_ms =
					static_cast<int>(std::clamp(18.0f - error * 2.0f, 8.0f, 18.0f));
				m_delay_time = std::max(m_delay_time, now + settle_delay_ms);
				return;
			}
		}

		m_delay_time = 0;
		return;
	}

	if (m_only_accurate_shots &&
		(!game_info.controlled_player.on_ground ||
			horizontal_speed > distance_speed_limit ||
			(m_last_automatic_shot_at > 0 &&
				(now - m_last_automatic_shot_at) < 300 &&
				(recoil_magnitude > 0.04f ||
					view_punch_magnitude > 0.04f))))
	{
		m_head_lock_started_at = 0;
		if (m_auto_stop && game_info.controlled_player.on_ground)
		{
			const float yaw =
				game_info.controlled_player.view_vec.y * PI / 180.0f;
			const float forward_speed =
				game_info.controlled_player.velocity.x * std::cos(yaw) +
				game_info.controlled_player.velocity.y * std::sin(yaw);
			const float right_speed =
				game_info.controlled_player.velocity.x * -std::sin(yaw) +
				game_info.controlled_player.velocity.y * std::cos(yaw);

			Movement counter{};
			counter.backward = forward_speed > distance_speed_limit;
			counter.forward = forward_speed < -distance_speed_limit;
			counter.left = right_speed > distance_speed_limit;
			counter.right = right_speed < -distance_speed_limit;
			handler->set_counter_strafe(counter);
		}
		else
		{
			handler->set_counter_strafe(Movement{});
		}
		return;
	}

	handler->set_counter_strafe(Movement{});

	const int target_entity_index = game_info.player_in_crosshair->entity_index;
	if (target_entity_index != m_current_target_entity_index)
	{
		m_current_target_entity_index = target_entity_index;
		m_head_lock_started_at = now;
		m_delay_time = 0;
	}

	if (m_head_lock_started_at == 0)
		m_head_lock_started_at = now;

	// Confirm a stable lock for a short period before firing.
	// This reduces edge-of-head snapshots and micro-flick misses.
	const int lock_confirmation_ms =
		static_cast<int>(std::clamp(target_distance / 45.0f, 8.0f, 28.0f));
	if ((now - m_head_lock_started_at) < lock_confirmation_ms)
		return;

	// If recoil/view punch is still settling, wait one more sample.
	const float settle_gate = std::clamp(0.02f + target_distance / 120000.0f, 0.02f, 0.045f);
	if (recoil_magnitude > settle_gate || view_punch_magnitude > settle_gate)
		return;

	// Do not shoot on the same sample that moved the view angle. Require the
	// Aimbot data has already been refreshed after writing the view angle.
	// Keep a short confirmation window for a fast single shot after lock.
	if (now < m_delay_time)
		return;

	// A real mouse hold already controls the weapon; do not interrupt it.
	if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)
		return;

	handler->click_player_weapon();
	m_last_automatic_shot_at = now;
	const int distance_delay =
		static_cast<int>(std::clamp(target_distance / 10.0f, 75.0f, 240.0f));
	const int recoil_delay =
		static_cast<int>(std::clamp(recoil_magnitude * 90.0f, 0.0f, 120.0f));
	m_delay_time =
		now + std::max({ 70, m_time_between_shots, distance_delay, recoil_delay });
	m_head_lock_started_at = 0;
}

void Triggerbot::set_shoot_at_teammates(bool shoot_at_teammates)
{
	m_shoot_at_teammates = shoot_at_teammates;
}

void Triggerbot::set_time_between_shots(int milliseconds)
{
	m_time_between_shots = milliseconds;
}

void Triggerbot::set_head_only(bool head_only, float tolerance_degrees)
{
	m_head_only = head_only;
	m_head_tolerance_degrees = std::clamp(tolerance_degrees, 0.01f, 5.0f);
}

void Triggerbot::set_accuracy_gate(bool enabled, float max_speed, bool auto_stop)
{
	m_only_accurate_shots = enabled;
	m_max_shot_speed = std::clamp(max_speed, 0.0f, 350.0f);
	m_auto_stop = auto_stop;
}

void Triggerbot::set_target_offsets(float z_offset, float left_offset)
{
	m_target_z_offset = std::clamp(z_offset, -30.0f, 20.0f);
	m_target_left_offset = std::clamp(left_offset, -10.0f, 10.0f);
}

void Triggerbot::set_recoil_compensation(float compensation)
{
	m_recoil_compensation = std::clamp(compensation, 0.0f, 3.0f);
}

void Triggerbot::reset()
{
	m_current_target_entity_index = -1;
	m_head_lock_started_at = 0;
	m_delay_time = 0;
	m_last_automatic_shot_at = 0;
}

bool Triggerbot::is_aimed_at_head(const GameInformation& game_info) const
{
	if (!game_info.player_in_crosshair)
		return false;

	Vec3D<float> target_point = game_info.player_in_crosshair->head_position;
	const float target_distance =
		game_info.controlled_player.head_position.distance(target_point);
	const float screen_pitch = game_info.controlled_player.view_vec.x;
	const float screen_yaw = game_info.controlled_player.view_vec.y;

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

	Vec2D<float> desired_view =
		calc_view_vec_to_point(game_info.controlled_player.head_position, target_point);
	desired_view.x -=
		game_info.controlled_player.aim_punch.x * m_recoil_compensation;
	desired_view.y =
		normalize_yaw(
			desired_view.y -
			game_info.controlled_player.aim_punch.y * m_recoil_compensation);

	float raw_yaw_delta = normalize_yaw(desired_view.y - screen_yaw);
	const float raw_pitch_delta =
		desired_view.x - screen_pitch;
	const float raw_crosshair_error =
		std::sqrt(
			raw_pitch_delta * raw_pitch_delta +
			raw_yaw_delta * raw_yaw_delta);
	const float raw_head_radius =
		std::clamp(
			std::atan2(
				game_info.player_in_crosshair->head_radius,
				std::max(target_distance, 1.0f)) /
				PI *
				180.0f,
			0.025f,
			0.16f);
	const float allowed_error =
		std::min(
			raw_head_radius,
			std::max(
				m_head_tolerance_degrees,
				raw_head_radius * 0.65f));
	const float strict_center_error =
		std::max(
			0.01f,
			std::min(
				allowed_error,
				raw_head_radius * 0.55f));
	// A positive delta means the screen aim is above the head center.
	if (raw_pitch_delta > strict_center_error * 0.6f)
		return false;
	return raw_crosshair_error <= strict_center_error;
}
