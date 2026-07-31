#include "pch.hpp"

bool f::players::get_data(int32_t idx, c_cs_player_controller* player, c_cs_player_pawn* player_pawn)
{
	if (!player || !player_pawn || !sdk::m_local_controller)
		return false;

	const auto local_pawn = sdk::m_local_controller->get_player_pawn();
	if (!local_pawn)
		return false;

	const auto health = player_pawn->m_iHealth();
	const auto is_dead = health <= 0;
	const auto vec_origin = player_pawn->get_scene_origin();
	const auto team = player->m_iTeamNum();
	const auto local_team = sdk::m_local_controller->m_iTeamNum();
	const auto local_origin = local_pawn->get_scene_origin();
	const auto dx = vec_origin.m_x - local_origin.m_x;
	const auto dy = vec_origin.m_y - local_origin.m_y;
	const auto dz = vec_origin.m_z - local_origin.m_z;
	const auto distance_to_local = std::sqrt(dx * dx + dy * dy + dz * dz);

	const auto money_services = player->m_pInGameMoneyServices();
	const auto item_services = player_pawn->m_pItemServices();
	const bool has_item_services = item_services != nullptr;

	m_player_data["m_idx"] = idx;
	m_player_data["m_name"] = player->m_sSanitizedPlayerName();
	m_player_data["m_color"] = player->get_color();
	m_player_data["m_team"] = team;
	m_player_data["m_health"] = health;
	m_player_data["m_is_dead"] = is_dead;
	m_player_data["m_is_local"] = player == sdk::m_local_controller;
	m_player_data["m_is_enemy"] =
		team != local_team &&
		team != e_team::none &&
		team != e_team::spec;
	m_player_data["m_distance_to_local"] = distance_to_local;
	m_player_data["m_model_name"] = player_pawn->get_model_name();
	m_player_data["m_steam_id"] = std::to_string(player->m_steamID());
	m_player_data["m_money"] = money_services ? money_services->m_iAccount() : 0;
	m_player_data["m_armor"] = player_pawn->m_ArmorValue();
	m_player_data["m_position"]["x"] = vec_origin.m_x;
	m_player_data["m_position"]["y"] = vec_origin.m_y;
	m_player_data["m_position"]["z"] = vec_origin.m_z;
	m_player_data["m_eye_angle"] = player_pawn->m_angEyeAngles().m_y;
	m_player_data["m_has_helmet"] = has_item_services && item_services->m_bHasHelmet();
	m_player_data["m_has_defuser"] = has_item_services && item_services->m_bHasDefuser();
	m_player_data["m_has_bomb"] = false;
	m_player_data["m_weapons"] = nlohmann::json{};

	if (team == e_team::t && !is_dead)
		m_player_data["m_has_bomb"] = m_bomb_idx == (player->m_hPawn().get_entry_idx() & 0xffff);

	return true;
}

void f::players::get_weapons(c_cs_player_pawn* player_pawn)
{
	const auto weapon_services = player_pawn->m_pWeaponServices();
	if (!weapon_services)
		return;

	const auto my_weapons = weapon_services->m_hMyWeapons();
	if (!my_weapons.m_size)
		return;

	std::set<std::string> utilities_set{};
	std::set<std::string> melee_set{};

	for (size_t idx{ 0 }; idx < my_weapons.m_size; idx++)
	{
		const auto weapon = my_weapons.m_elements->get(idx);
		if (!weapon)
			continue;

		const auto weapon_data = weapon->m_WeaponData();
		if (!weapon_data)
			continue;

		auto weapon_name = weapon_data->m_szName();
		if (weapon_name.empty())
			continue;

		weapon_name.erase(weapon_name.begin(), weapon_name.begin() + 7);

		const auto weapon_type = weapon_data->m_WeaponType();
		switch (weapon_type)
		{
			case e_weapon_type::submachinegun:
			case e_weapon_type::rifle:
			case e_weapon_type::shotgun:
			case e_weapon_type::sniper_rifle:
			case e_weapon_type::machinegun:
				m_player_data["m_weapons"]["m_primary"] = weapon_name;
				break;

			case e_weapon_type::pistol:
				m_player_data["m_weapons"]["m_secondary"] = weapon_name;
				break;

			case e_weapon_type::knife:
			case e_weapon_type::taser:
				melee_set.insert(weapon_name);
				break;

			case e_weapon_type::grenade:
				utilities_set.insert(weapon_name);
				break;
		}
	}

	m_player_data["m_weapons"]["m_melee"] = std::vector<std::string>(melee_set.begin(), melee_set.end());
	m_player_data["m_weapons"]["m_utilities"] = std::vector<std::string>(utilities_set.begin(), utilities_set.end());
}

void f::players::get_active_weapon(c_cs_player_pawn* player_pawn)
{
	const auto weapon_services = player_pawn->m_pWeaponServices();
	if (!weapon_services)
		return;

	const auto weapon_handle = weapon_services->m_hActiveWeapon();
	if (!weapon_handle.is_valid())
		return;

	const auto active_weapon = i::m_game_entity_system->get<c_base_player_weapon*>(weapon_handle);
	if (!active_weapon)
		return;

	const auto weapon_data = active_weapon->m_WeaponData();
	if (!weapon_data)
		return;

	auto weapon_name = weapon_data->m_szName();
	if (weapon_name.empty())
		return;

	weapon_name.erase(weapon_name.begin(), weapon_name.begin() + 7);

	m_player_data["m_weapons"]["m_active"] = weapon_name;
}
