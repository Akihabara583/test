#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include "CS2/CS2AI.h"
#include "Utility/Logger.h"
#include "Utility/Logging.h"
#include "Utility/json.hpp"

class ConsoleLogger final : public Logger
{
public:
	void log(const std::string& str) override
	{
		std::cout << "[AI] " << str << '\n';
	}

	void log_error(const std::string& str) override
	{
		std::cerr << "[AI] ERROR: " << str << '\n';
	}

	void log_success(const std::string& str) override
	{
		std::cout << "[AI] OK: " << str << '\n';
	}
};

struct HeadlessSettings
{
	bool start_enabled = false;
	bool aimbot = true;
	bool triggerbot = true;
	bool movement = false;
	int trigger_delay_ms = 120;
	bool ignore_same_team = true;
	bool visible_targets_only = true;
	float aim_fov_degrees = 7.5f;
	float aim_smoothing = 0.32f;
	float aim_max_step_degrees = 1.8f;
	float aim_target_z_offset = 2.5f;
	float aim_target_left_offset = 1.2f;
	int manual_mouse_override_ms = 140;
	bool trigger_head_only = true;
	float trigger_head_tolerance_degrees = 0.03f;
	float recoil_compensation = 2.0f;
	bool only_accurate_shots = true;
	float max_shot_speed = 500.0f;
	bool auto_stop = true;
};

static HeadlessSettings load_settings()
{
	HeadlessSettings result{};
	try
	{
		std::ifstream stream("Configuration/ai_config.json");
		if (!stream)
			return result;

		const auto json = nlohmann::json::parse(stream);
		result.start_enabled = json.value("start_enabled", false);
		result.aimbot = json.value("aimbot", result.aimbot);
		result.triggerbot = json.value("triggerbot", result.triggerbot);
		result.movement = json.value("movement", result.movement);
		result.trigger_delay_ms =
			std::clamp(json.value("trigger_delay_ms", result.trigger_delay_ms), 0, 5000);
		result.ignore_same_team =
			json.value("ignore_same_team", result.ignore_same_team);
		result.visible_targets_only =
			json.value("visible_targets_only", result.visible_targets_only);
		result.aim_fov_degrees =
			std::clamp(json.value("aim_fov_degrees", result.aim_fov_degrees), 0.5f, 90.0f);
		result.aim_smoothing =
			std::clamp(json.value("aim_smoothing", result.aim_smoothing), 0.01f, 1.0f);
		result.aim_max_step_degrees =
			std::clamp(json.value("aim_max_step_degrees", result.aim_max_step_degrees), 0.05f, 180.0f);
		result.aim_target_z_offset =
			std::clamp(json.value("aim_target_z_offset", result.aim_target_z_offset), -30.0f, 20.0f);
		result.aim_target_left_offset =
			std::clamp(
				json.value("aim_target_left_offset", result.aim_target_left_offset),
				-10.0f,
				10.0f);
		result.manual_mouse_override_ms =
			std::clamp(json.value("manual_mouse_override_ms", result.manual_mouse_override_ms), 0, 2000);
		result.trigger_head_only =
			json.value("trigger_head_only", result.trigger_head_only);
		result.trigger_head_tolerance_degrees =
			std::clamp(
				json.value(
					"trigger_head_tolerance_degrees",
					result.trigger_head_tolerance_degrees),
				0.01f,
				5.0f);
		result.recoil_compensation =
			std::clamp(
				json.value("recoil_compensation", result.recoil_compensation),
				0.0f,
				3.0f);
		result.only_accurate_shots =
			json.value("only_accurate_shots", result.only_accurate_shots);
		result.max_shot_speed =
			std::clamp(
				json.value("max_shot_speed", result.max_shot_speed),
				0.0f,
				500.0f);
		result.auto_stop = json.value("auto_stop", result.auto_stop);
	}
	catch (const std::exception& error)
	{
		std::cerr << "[AI] Invalid Configuration/ai_config.json: "
			<< error.what() << '\n';
	}
	return result;
}

int main()
{
	SetConsoleTitleA("CS2 AI");
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

	ConsoleLogger logger;
	Logging::set_logger(&logger);

	const HeadlessSettings settings = load_settings();
	Config config{};
	config.windowname = "Counter-Strike 2";
	config.client_dll_name = "client.dll";
	config.engine_dll_name = "engine2.dll";
	config.delay_between_shots = settings.trigger_delay_ms;
	config.ignore_same_team = settings.ignore_same_team;
	config.visible_targets_only = settings.visible_targets_only;
	config.aim_fov_degrees = settings.aim_fov_degrees;
	config.aim_smoothing = settings.aim_smoothing;
	config.aim_max_step_degrees = settings.aim_max_step_degrees;
	config.aim_target_z_offset = settings.aim_target_z_offset;
	config.aim_target_left_offset = settings.aim_target_left_offset;
	config.manual_mouse_override_ms = settings.manual_mouse_override_ms;
	config.trigger_head_only = settings.trigger_head_only;
	config.trigger_head_tolerance_degrees = settings.trigger_head_tolerance_degrees;
	config.recoil_compensation = settings.recoil_compensation;
	config.only_accurate_shots = settings.only_accurate_shots;
	config.max_shot_speed = settings.max_shot_speed;
	config.auto_stop = settings.auto_stop;

	CS2Ai ai;
	ai.set_config(config);
	if (!ai.load_offsets())
	{
		std::cerr << "[AI] Offsets were not loaded. Check Configuration.\n";
		return 1;
	}

	std::cout << "[AI] Ready and disabled. Press 9 or Numpad 9 to toggle.\n";
	std::cout << "[AI] Aimbot=" << settings.aimbot
		<< " Triggerbot=" << settings.triggerbot
		<< " Movement=" << settings.movement << '\n';

	bool enabled = settings.start_enabled;
	if (enabled)
	{
		ActivatedFeatures features{};
		features.aimbot = settings.aimbot;
		features.triggerBot = settings.triggerbot;
		features.movement = settings.movement;
		ai.set_activated_behavior(features);
	}
	bool nine_was_down = false;
	ULONGLONG next_attach_attempt = 0;

	for (;;)
	{
		const bool nine_is_down =
			(GetAsyncKeyState('9') & 0x8000) != 0 ||
			(GetAsyncKeyState(VK_NUMPAD9) & 0x8000) != 0;

		if (nine_is_down && !nine_was_down)
		{
			enabled = !enabled;
			if (enabled)
			{
				ActivatedFeatures features{};
				features.aimbot = settings.aimbot;
				features.triggerBot = settings.triggerbot;
				features.movement = settings.movement;
				ai.set_activated_behavior(features);
			}
			else
			{
				ai.stop_actions();
			}
			std::cout << "[AI] " << (enabled ? "enabled" : "disabled") << '\n';
		}
		nine_was_down = nine_is_down;

		const bool attached = ai.get_game_info_handler()->is_attached_to_process();
		if (!attached && GetTickCount64() >= next_attach_attempt)
		{
			if (ai.attach_to_cs2_process())
				std::cout << "[AI] Connected to CS2.\n";
			next_attach_attempt = GetTickCount64() + 1000;
		}

		if (enabled && ai.get_game_info_handler()->is_attached_to_process())
			ai.update();

		Sleep(enabled ? 1 : 50);
	}
}
