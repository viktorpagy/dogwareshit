#include "../includes.h"

#include "../UTILS/interfaces.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/IEngine.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/ConVar.h"
#include "../SDK/ISurface.h"
#include "../UTILS/render.h"

#include "../FEATURES/Backtracking.h"
#include "../FEATURES/Resolver.h"
#include "../FEATURES/Visuals.h"
#include "../FEATURES/Aimbot.h"

#include "EventListener.h"
#include <playsoundapi.h>

#pragma comment(lib, "Winmm.lib")

CGameEvents::ItemPurchaseListener item_purchase_listener;
CGameEvents::PlayerHurtListener player_hurt_listener;
CGameEvents::BulletImpactListener bullet_impact_listener;
CGameEvents::AntiAimDisableListener antiaim_disable_listener;
CGameEvents::RoundStartListener round_start_listener;
HitmarkerStuff* hitmarkerstuff;

unsigned int EpochMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
void CGameEvents::InitializeEventListeners()
{
	INTERFACES::GameEventManager->AddListener(&item_purchase_listener, "item_purchase", false);
	INTERFACES::GameEventManager->AddListener(&player_hurt_listener, "player_hurt", false);
	INTERFACES::GameEventManager->AddListener(&bullet_impact_listener, "bullet_impact", false);
	INTERFACES::GameEventManager->AddListener(&antiaim_disable_listener, "round_prestart", false);
	INTERFACES::GameEventManager->AddListener(&antiaim_disable_listener, "round_end", false);
	INTERFACES::GameEventManager->AddListener(&antiaim_disable_listener, "round_freeze_end", false);
	INTERFACES::GameEventManager->AddListener(&round_start_listener, ("round_start"), false);
}



void CGameEvents::AntiAimDisableListener::FireGameEvent(SDK::IGameEvent* game_event)
{
	if (!game_event) return;

	if (!(INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame()))
		return;

	if (!strcmp(game_event->GetName(), "round_freeze_end"))
		GLOBAL::DisableAA = false;

	if (!strcmp(game_event->GetName(), "round_prestart") || !strcmp(game_event->GetName(), "round_end"))
         GLOBAL::DisableAA = true;
}

char* HitgroupToName(int hitgroup)
{
	switch (hitgroup)
	{
	case HITGROUP_HEAD:
		return "head";
	case HITGROUP_LEFTLEG:
		return "left leg";
	case HITGROUP_RIGHTLEG:
		return "right leg";
	case HITGROUP_STOMACH:
		return "stomach";
	case HITGROUP_LEFTARM:
		return "left arm";
	case HITGROUP_RIGHTARM:
		return "right arm";
	default:
		return "body";
	}
}

void CGameEvents::PlayerHurtListener::FireGameEvent(SDK::IGameEvent* game_event)
{
	if (!game_event)
		return;

	if (!(INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame()))
		return;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	auto victim = INTERFACES::Engine->GetPlayerForUserID(game_event->GetInt("userid")); if (!victim) return;
	auto entity = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetPlayerForUserID(game_event->GetInt("userid")));
	if (!entity)
		return;

	auto entity_attacker = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetPlayerForUserID(game_event->GetInt("attacker")));
	if (!entity_attacker)
		return;

	SDK::player_info_t player_info;
	if (!INTERFACES::Engine->GetPlayerInfo(entity->GetIndex(), &player_info))
		return;

	if (entity_attacker == local_player)
	{
		didMiss = false;

		visuals->set_hitmarker_time( INTERFACES::Globals->curtime );
		switch (SETTINGS::settings.hitmarker_val)
		{
			case 0: break; // no hitsound nigga
			case 1: INTERFACES::Surface->IPlaySound("buttons\\arena_switch_press_02.wav"); break; // skeet.cc not overrated
			case 2: PlaySound("C:\\DogWare\\hitmarker.wav", NULL, SND_ASYNC); break; // custom
		}
		if (SETTINGS::settings.logs)
		{
			auto pVictim = reinterpret_cast<SDK::CBaseEntity*>(INTERFACES::ClientEntityList->GetClientEntity(victim));
			if (!pVictim) return;
			SDK::player_info_t pinfo;
			INTERFACES::Engine->GetPlayerInfo(pVictim->GetIndex(), &pinfo);

			auto hitbox = game_event->GetInt("hitgroup");
			if (!hitbox) return;

			auto damage = game_event->GetInt("dmg_health");
			if (!damage) return;

			auto health = game_event->GetInt("health");
			if (!health && health != 0) return;

			auto hitgroup = HitgroupToName(hitbox);

			GLOBAL::Msg("[DogWare] Hit %s in the %s for %d damage (%d health remaining).     \n", pinfo.name, hitgroup, damage, health);

			// memes
			if (SETTINGS::settings.trashtalk) {
				if (damage >= 100) {
					INTERFACES::Engine->ClientCmd_Unrestricted("say 1");
				}
			}

			//if hit head
			if (SETTINGS::settings.trashtalk2) {
				if (hitbox == 1) {
					INTERFACES::Engine->ClientCmd_Unrestricted("say head on my screen cya");
				}
			}
		}
		shots_hit[entity->GetIndex()]++;
		hurtcalled = true;
	}
}

void CGameEvents::BulletImpactListener::FireGameEvent(SDK::IGameEvent* game_event)
{
	if (!game_event)
		return;

	if (!(INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame()))
		return;

	int iUser = INTERFACES::Engine->GetPlayerForUserID(game_event->GetInt("userid"));
	auto entity = reinterpret_cast<SDK::CBaseEntity*>(INTERFACES::ClientEntityList->GetClientEntity(iUser));
	if (!entity) return;

	if (entity->GetIsDormant()) return;

	float x, y, z;
	x = game_event->GetFloat("x");
	y = game_event->GetFloat("y");
	z = game_event->GetFloat("z");
	
	UTILS::BulletImpact_t impact(entity, Vector(x, y, z), INTERFACES::Globals->curtime, iUser == INTERFACES::Engine->GetLocalPlayer() ? GREEN : RED);

	visuals->Impacts.push_back(impact);
}

void  CGameEvents::RoundStartListener::FireGameEvent(SDK::IGameEvent* game_event)
{
	if (!game_event)
		return;

	if (!(INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame()))
		return;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	if (!SETTINGS::settings.buybot_enabled)
		return;

	switch (SETTINGS::settings.buybot_rifle)
	{
	case 1:
		INTERFACES::Engine->ClientCmd("buy scar20;buy g3sg1");
		break;
	case 2:
		INTERFACES::Engine->ClientCmd("buy ssg08");
		break;
	case 3:
		INTERFACES::Engine->ClientCmd("buy awp");
		break;
	default:
		break;
	}

	switch (SETTINGS::settings.buybot_pistol)
	{
	case 1:
		INTERFACES::Engine->ClientCmd("buy deagle");
		break;
	case 2:
		INTERFACES::Engine->ClientCmd("buy elite");
		break;
	case 3:
		INTERFACES::Engine->ClientCmd("buy p250");
		break;
	default:
		break;
	}

	switch (SETTINGS::settings.buybot_armor)
	{
	case 1:
		INTERFACES::Engine->ClientCmd("buy vest");
		break;
	case 2:
		INTERFACES::Engine->ClientCmd("buy vesthelm");
		break;
	default:
		break;
	}

	if (SETTINGS::settings.buybot_grenade)
		INTERFACES::Engine->ClientCmd("buy hegrenade; buy molotov; buy incgrenade; buy smokegrenade;");



	if (SETTINGS::settings.buybot_zeus)
		INTERFACES::Engine->ClientCmd("buy taser 34");
}


void CGameEvents::ItemPurchaseListener::FireGameEvent(SDK::IGameEvent* game_event)
{
	if (!game_event)return;
	if (!(INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame()))return;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)return;

	auto entity = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetPlayerForUserID(game_event->GetInt("userid")));
	if (!entity)return;

	if (entity->GetTeam() == local_player->GetTeam())return;

	SDK::player_info_t player_info;
	if (!INTERFACES::Engine->GetPlayerInfo(entity->GetIndex(), &player_info))return;

	auto event_weapon = game_event->GetString("weapon");

	if (event_weapon = "weapon_unknown")return;
	if (!event_weapon)return;

	if (SETTINGS::settings.esp_bool)
	{
		GLOBAL::Msg("[DogWare] %s bought %s.     \n", player_info.name, event_weapon);
	}
};