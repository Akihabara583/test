#pragma once
#include <string>

struct Config 
{
	std::string client_dll_name;
	std::string engine_dll_name;
	std::string  windowname;
	int delay_between_shots = 28;
	bool ignore_same_team = true;
	bool visible_targets_only = true;
	float aim_fov_degrees = 24.0f;
	float aim_smoothing = 1.0f;
	float aim_max_step_degrees = 24.0f;
	float aim_target_z_offset = -1.6f;
	float aim_target_left_offset = 1.2f;
	int manual_mouse_override_ms = 0;
	bool trigger_head_only = true;
	float trigger_head_tolerance_degrees = 0.055f;
	float recoil_compensation = 1.45f;
	bool only_accurate_shots = false;
	float max_shot_speed = 500.0f;
	bool auto_stop = false;
	//DWORD trigger_button = 0;
};
