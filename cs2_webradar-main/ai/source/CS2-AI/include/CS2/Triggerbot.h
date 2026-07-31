#pragma once
#include "Utility/Utility.h"
#include "GameInformationHandler.h"

class Triggerbot 
{
public:
	void update(GameInformationhandler* handler);
	void set_shoot_at_teammates(bool shoot_at_teammates);
	void set_time_between_shots(int milliseconds);
	void set_head_only(bool head_only, float tolerance_degrees);
	void set_accuracy_gate(bool enabled, float max_speed, bool auto_stop);
	void set_target_offsets(float z_offset, float left_offset);
	void set_recoil_compensation(float compensation);
	void reset();

private:
	bool is_aimed_at_head(const GameInformation& game_info) const;
	long long m_delay_time = 0;
	long long m_last_automatic_shot_at = 0;
	bool m_shoot_at_teammates = false;
	int m_current_target_entity_index = -1;
	long long m_head_lock_started_at = 0;
	long long m_manual_override_until = 0;
	bool m_head_only = true;
	float m_head_tolerance_degrees = 0.55f;
	bool m_only_accurate_shots = true;
	float m_max_shot_speed = 500.0f;
	bool m_auto_stop = false;
	float m_target_z_offset = 0.0f;
	float m_target_left_offset = 0.0f;
	float m_recoil_compensation = 0.0f;
	bool m_last_write_pending = false;
	Vec2D<float> m_last_written_view{};
	int m_time_between_shots = 85;
};
