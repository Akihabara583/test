#pragma once
#include <string>

struct Config 
{
	std::string client_dll_name;
	std::string engine_dll_name;
	std::string  windowname;
	int delay_between_shots = 0;
	bool ignore_same_team = true;
	bool visible_targets_only = true;
	float aim_fov_degrees = 9.0f;
	float aim_smoothing = 0.48f;
	float aim_max_step_degrees = 2.6f;
	float aim_target_z_offset = 2.5f;
	float aim_target_left_offset = 1.2f;
	int manual_mouse_override_ms = 80;
	bool trigger_head_only = true;
	float trigger_head_tolerance_degrees = 0.03f;
	float recoil_compensation = 2.0f;
	bool only_accurate_shots = true;
	float max_shot_speed = 350.0f;
	bool auto_stop = true;
	//DWORD trigger_button = 0;
};
