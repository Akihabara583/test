#pragma once
#include <chrono>
#include <math.h>
#include "GameInformationHandler.h"

class Aimbot
{
public:
	void update(GameInformationhandler* info_handler);
	void set_config(const Config& config);

private:
	Vec2D<float> calc_view_vec_aim_to_head(const Vec3D<float>& player_head, const Vec3D<float>& enemy_head);
	static float normalize_yaw(float angle);
	static Vec2D<float> angle_delta(const Vec2D<float>& from, const Vec2D<float>& to);
	static float angle_distance(const Vec2D<float>& first, const Vec2D<float>& second);
	bool m_aim_at_teammates = false;
	bool m_visible_targets_only = true;
	float m_fov_degrees = 6.0f;
	float m_smoothing = 0.18f;
	float m_max_step_degrees = 0.40f;
	float m_target_z_offset = -4.0f;
	float m_target_left_offset = 0.0f;
	float m_recoil_compensation = 2.0f;
	int m_manual_override_ms = 140;
	bool m_last_write_pending = false;
	Vec2D<float> m_last_written_view{};
	std::chrono::steady_clock::time_point m_manual_override_until{};
};
