#pragma once
#include <iostream>
#include <vector>
#include <optional>
#include <chrono>
#include "Offsets.h"
#include "Utility/Vec3D.h"
#include "Utility/Vec2D.h"
#include "MemoryManager.h"
#include "Config.h"

struct Movement
{
	bool forward = false;
	bool backward = false;
	bool left = false;
	bool right = false;
	bool jump = false;
};

struct ControlledPlayer 
{
	Vec2D<float> view_vec;
	Vec2D<float> aim_punch;
	Vec2D<float> recoil_signal;
	Vec2D<float> view_punch;
	Vec3D<float> position;
	Vec3D<float> head_position;
	Vec3D<float> velocity;
	Movement movement;
	DWORD shooting;
	DWORD shots_fired;
	int team;
	int health;
	int entity_index;
	bool on_ground;
};

struct PlayerInformation 
{
	Vec3D<float> position;
	Vec3D<float> head_position;
	int team;
	int health;
	int entity_index;
	bool visible;
	float head_radius = 2.5f;
};

struct GameInformation 
{
	ControlledPlayer controlled_player;
	std::vector<PlayerInformation> other_players;
	std::optional<PlayerInformation> player_in_crosshair;
	std::optional<PlayerInformation> closest_target_player; // Can be in the same team, or enemy team only
	std::string current_map = "";
};

class GameInformationhandler
{
public:
	GameInformationhandler() = default;
	bool init(const Config& config);
	bool loadOffsets();
	void update_game_information();

	GameInformation get_game_information() const;
	bool is_attached_to_process()const;
	void set_view_vec(const Vec2D<float>& view_vec);
	void set_player_movement(const Movement& movement);
	void set_counter_strafe(const Movement& movement);
	void set_player_shooting(bool val);
	void click_player_weapon();
	void stop_actions();
	void set_config(Config config);

private:
	ControlledPlayer read_controlled_player_information(uintptr_t player_address);
	std::vector<PlayerInformation> read_other_players(uintptr_t player_address, int controlled_entity_index);
	Movement read_controlled_player_movement(uintptr_t player_address);
	Vec3D<float> get_bone_position(uintptr_t player_pawn, DWORD bone_index);
	uintptr_t get_list_entity(uintptr_t id, uintptr_t entity_list);
	uintptr_t get_entity_controller_or_pawn(uintptr_t list_entity, uintptr_t id);
	int find_controller_entity_index(uintptr_t entity_list, uintptr_t controller_address);
	std::optional<PlayerInformation> read_player(uintptr_t entity_list_begin, uintptr_t id, uintptr_t player_address, int controlled_entity_index);
	std::optional<PlayerInformation> read_player_in_crosshair(uintptr_t player_controller, uintptr_t player_pawn, int controlled_entity_index);
	void scan_active_smokes_batch(uintptr_t entity_list);
	static bool line_passes_through_smoke(
		const Vec3D<float>& start,
		const Vec3D<float>& end,
		const std::vector<Vec3D<float>>& smoke_centers);
	std::optional<PlayerInformation> get_closest_player(const GameInformation& game_info, bool only_enemy_team);
	std::string read_in_current_map();
	bool read_in_if_controlled_player_is_shooting();

	bool m_attached_to_process = false;
	GameInformation m_game_information;
	MemoryManager m_process_memory;
	uintptr_t m_client_dll_address = 0;
	Offsets m_offsets = {};
	Config m_config = {};
	Movement m_counter_strafe{};
	std::vector<Vec3D<float>> m_active_smokes;
	std::vector<Vec3D<float>> m_pending_smokes;
	int m_next_smoke_entity_index = 65;
};
