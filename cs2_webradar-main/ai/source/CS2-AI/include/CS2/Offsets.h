#pragma once
#include <wtypes.h>
#include <fstream>
#include <optional>
#include "Utility/json.hpp"
#include "Utility/Logging.h"

struct Offsets
{
public:
	//Will be read from the offsets files
	uintptr_t local_player_controller_offset;
	uintptr_t local_player_pawn;
	uintptr_t crosshair_offset;
	uintptr_t shots_fired_offset;
	uintptr_t entity_list_start_offset;
	uintptr_t highest_entity_index;
	uintptr_t player_health_offset;
	uintptr_t team_offset;
	uintptr_t entity_listelement_size;
	uintptr_t position;
	uintptr_t view_offset;
	uintptr_t velocity;
	uintptr_t flags;
	uintptr_t force_attack;
	uintptr_t force_forward;
	uintptr_t force_backward;
	uintptr_t force_left;
	uintptr_t force_right;
	uintptr_t model_state;
	uintptr_t sceneNode;
	uintptr_t player_pawn_handle;
	uintptr_t entity_spotted_state;
	uintptr_t spotted_by_mask;
	uintptr_t aim_punch_services;
	uintptr_t aim_punch_angle;
	uintptr_t predictable_aim_punch_angle;
	uintptr_t camera_services;
	uintptr_t view_punch_angle;
	uintptr_t entity_identity;
	uintptr_t entity_designer_name;
	uintptr_t smoke_did_effect;
	uintptr_t smoke_detonation_position;
	uintptr_t client_state_view_angle;
	uintptr_t current_map;
	uintptr_t global_vars;
};

std::optional <Offsets> load_offsets_from_files();
