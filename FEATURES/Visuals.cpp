#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/ConVar.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/IViewRenderBeams.h"
#include "../FEATURES/Backtracking.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CBaseWeapon.h"
#include "../FEATURES/AutoWall.h"
#include "../SDK/CTrace.h"	
#include "../FEATURES/Resolver.h"
#include "../SDK/CGlobalVars.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Visuals.h"
#include "../UTILS/render.h"
#include "../SDK/IVDebugOverlay.h"
#include <string.h>

//--- Misc Variable Initalization ---//
float flPlayerAlpha[255];
CColor breaking;
CColor backtrack;
static bool bPerformed = false, bLastSetting;
float fade_alpha[65];
float dormant_time[65];
CColor main_color;
CColor ammo;
#define clamp(val, min, max) (((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))

std::string fix_item_name(std::string name)
{
	if (name[0] == 'C')
		name.erase(name.begin());

	auto startOfWeap = name.find("Weapon");
	if (startOfWeap != std::string::npos)
		name.erase(name.begin() + startOfWeap, name.begin() + startOfWeap + 6);

	return name;
}

float CVisuals::resolve_distance(Vector src, Vector dest)
{
	Vector delta = src - dest;
	float fl_dist = ::sqrtf((delta.Length()));
	if (fl_dist < 1.0f) return 1.0f;
	return fl_dist;
}

void CVisuals::set_hitmarker_time(float time)
{
	GLOBAL::flHurtTime = time;
}

void CVisuals::LogEvents()
{
	static bool convar_performed = false, convar_lastsetting;

	if (convar_lastsetting != SETTINGS::settings.info_bool)
	{
		convar_lastsetting = SETTINGS::settings.info_bool;
		convar_performed = false;
	}

	if (!convar_performed)
	{
		static auto developer = INTERFACES::cvar->FindVar("developer");
		developer->SetValue(1);
		static auto con_filter_text_out = INTERFACES::cvar->FindVar("con_filter_text_out");
		static auto con_filter_enable = INTERFACES::cvar->FindVar("con_filter_enable");
		static auto con_filter_text = INTERFACES::cvar->FindVar("con_filter_text");

		con_filter_text->SetValue(".     ");
		con_filter_text_out->SetValue("");
		con_filter_enable->SetValue(2);
		convar_performed = true;
	}
}

CVisuals world_textures;
bool prepared = false;
CColor world_color;
CColor prop_color;
CColor skybox_color;
float prop_alpha = 1.f;

void CVisuals::DoFSN()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	static bool world_performed = false, world_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
	{
		if (world_performed)
			world_performed = false;
		return;
	}

	if (world_performed != SETTINGS::settings.wolrd_enabled)
	{
		world_lastsetting = SETTINGS::settings.wolrd_enabled;
		world_performed = false;
	}

	if (!local_player)
	{
		if (world_performed)
			world_performed = false;
		return;
	}

	if (world_lastsetting != SETTINGS::settings.wolrd_enabled)
	{
		world_lastsetting = SETTINGS::settings.wolrd_enabled;
		world_performed = false;
	}

	if (!world_performed)
	{
		if (SETTINGS::settings.night_col != world_color)
		{
			for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i))
			{
				SDK::IMaterial *pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

				if (!pMaterial)
					continue;

				if (SETTINGS::settings.sky_enabled)
				{
					if (strstr(pMaterial->GetTextureGroupName(), "World textures") || strstr(pMaterial->GetTextureGroupName(), "StaticProp textures"))
					{
						pMaterial->ColorModulate(world_color);
					}
					else
					{
						pMaterial->ColorModulate(1, 1, 1);
					}
				}
			}
			world_color = SETTINGS::settings.night_col;
		}
	}

}


void CVisuals::Draw()
{
	if (!INTERFACES::Engine->IsInGame()) {
		GLOBAL::flHurtTime = 0.f;
		return;
	}
	DrawCrosshair();
	for (int i = 1; i <= 65; i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
		if (!entity) continue;
		if (!local_player) continue;

		bool is_local_player = entity == local_player;
		bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player) continue;
		if (entity->GetHealth() <= 0) continue;
		if (entity->GetVecOrigin() == Vector(0, 0, 0)) continue;

		//--- Colors ---//
		if (entity->GetIsDormant() && flPlayerAlpha[i] > 0) flPlayerAlpha[i] -= .3;
		else if (!entity->GetIsDormant() && flPlayerAlpha[i] < 255) flPlayerAlpha[i] = 255;

		float playerAlpha = flPlayerAlpha[i];
		int enemy_hp = entity->GetHealth();
		int hp_red = 255 - (enemy_hp * 2.55);
		int hp_green = enemy_hp * 2.55;
		CColor health_color = CColor(hp_red, hp_green, 1, playerAlpha);
		CColor dormant_color = CColor(100, 100, 100, playerAlpha);
		CColor box_color, still_health, alt_color, zoom_color, name_color, weapon_color, distance_color, arrow_col, lby;

		static auto alpha = 0.f; static auto plus_or_minus = false;
		if (alpha <= 0.f || alpha >= 255.f) plus_or_minus = !plus_or_minus;
		alpha += plus_or_minus ? (255.f / 7 * 0.015) : -(255.f / 7 * 0.015); alpha = clamp(alpha, 0.f, 255.f);
		float arrow_colour[3] = { SETTINGS::settings.fov_col.RGBA[0], SETTINGS::settings.fov_col.RGBA[1], SETTINGS::settings.fov_col.RGBA[2] };
		float arrowteam_colour[3] = { SETTINGS::settings.arrowteam_col.RGBA[0], SETTINGS::settings.arrowteam_col.RGBA[1], SETTINGS::settings.arrowteam_col.RGBA[2] };

		if (entity->GetIsDormant())
		{
			main_color = dormant_color;
			still_health = health_color;
			alt_color = CColor(20, 20, 20, playerAlpha);
			zoom_color = dormant_color;
			breaking = dormant_color;
			backtrack = dormant_color;
			box_color = dormant_color;
			ammo = dormant_color;
			name_color = dormant_color;
			weapon_color = dormant_color;
			distance_color = dormant_color;
			arrow_col = dormant_color;
			lby = dormant_color;
		}
		else if (!entity->GetIsDormant())
		{
			main_color = CColor(255, 255, 255, playerAlpha);
			still_health = health_color;
			alt_color = CColor(0, 0, 0, 165);
			zoom_color = CColor(150, 150, 220, 165);
			breaking = CColor(220, 150, 150, 165);
			backtrack = CColor(155, 220, 150, 165);
			box_color = SETTINGS::settings.box_col;
			ammo = CColor(61, 135, 255, 165);
			name_color = SETTINGS::settings.name_col;
			weapon_color = SETTINGS::settings.weapon_col;
			distance_color = SETTINGS::settings.distance_col;
			arrow_col = SETTINGS::settings.fov_col;
			lby = SETTINGS::settings.lbytimer_color;

		}
		Vector min, max, pos, pos3D, top, top3D; entity->GetRenderBounds(min, max);
		pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10); top3D = pos3D + Vector(0, 0, max.z + 10);

		if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
		{
			if (!is_teammate)
			{
				if (SETTINGS::settings.box_bool) DrawBox(entity, box_color, pos, top);
				if (SETTINGS::settings.name_bool) DrawName(entity, name_color, i, pos, top);
				if (SETTINGS::settings.weap_bool) DrawWeapon(entity, weapon_color, i, pos, top);
				if (SETTINGS::settings.health_bool) DrawHealth(entity, still_health, alt_color, pos, top);
				if (SETTINGS::settings.ammo_bool) DrawAmmo(entity, ammo, alt_color, pos, top);
				//if (SETTINGS::settings.lby_timer) DrawLBYTimer(entity, lby, alt_color);
			}
			else if (is_teammate)
			{
				if (SETTINGS::settings.boxteam) DrawBox(entity, SETTINGS::settings.boxteam_col, pos, top);
				if (SETTINGS::settings.nameteam) DrawName(entity, SETTINGS::settings.nameteam_col, i, pos, top);
				if (SETTINGS::settings.weaponteam) DrawWeapon(entity, SETTINGS::settings.weaponteam_col, i, pos, top);
				if (SETTINGS::settings.healthteam) DrawHealth(entity, still_health, alt_color, pos, top);
				if (SETTINGS::settings.ammoteam) DrawAmmo(entity, ammo, alt_color, pos, top);

			}
			DrawInfo(entity, main_color, zoom_color, pos, top);
		}
		if (!is_teammate)
		{
			if (SETTINGS::settings.fov_bool) DrawFovArrows(entity, CColor(arrow_colour[0], arrow_colour[1], arrow_colour[2], alpha));
		}
		else if (is_teammate)
		{
			if (SETTINGS::settings.arrowteam) DrawFovArrows(entity, CColor(arrowteam_colour[0], arrowteam_colour[1], arrowteam_colour[2], alpha));
		}
	}
}

void CVisuals::ClientDraw()
{
	if (SETTINGS::settings.spread_bool) DrawInaccuracy();
	if (SETTINGS::settings.scope_bool) DrawBorderLines();
	if (SETTINGS::settings.hitmarker_screen) DrawHitmarker();

	DrawIndicator();

	auto pLocal = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	static SDK::ConVar* crosshair = INTERFACES::cvar->FindVar("weapon_debug_spread_show");
	if (SETTINGS::settings.forcehair && !pLocal->GetIsScoped())
	{
		crosshair->SetValue(3);
	}
	else {
		crosshair->SetValue(0);
	}
}


void CVisuals::DrawBox(SDK::CBaseEntity* entity, CColor color, Vector pos, Vector top)
{
	float alpha = flPlayerAlpha[entity->GetIndex()];
	int height = (pos.y - top.y), width = height / 2;

	RENDER::DrawEmptyRect(pos.x - width / 2, top.y, (pos.x - width / 2) + width, top.y + height, color);
	RENDER::DrawEmptyRect((pos.x - width / 2) + 1, top.y + 1, (pos.x - width / 2) + width - 1, top.y + height - 1, CColor(20, 20, 20, alpha));
	RENDER::DrawEmptyRect((pos.x - width / 2) - 1, top.y - 1, (pos.x - width / 2) + width + 1, top.y + height + 1, CColor(20, 20, 20, alpha));
}

void CVisuals::DrawName(SDK::CBaseEntity* entity, CColor color, int index, Vector pos, Vector top)
{
	SDK::player_info_t ent_info; INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	int height = (pos.y - top.y), width = height / 2;
	RENDER::DrawF(pos.x, top.y - 7, FONTS::visuals_name_font, true, true, color, ent_info.name);
}

void CVisuals::DrawDistance(SDK::CBaseEntity* entity, CColor color, Vector pos, Vector top)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	SDK::player_info_t ent_info;
	Vector vecOrigin = entity->GetVecOrigin(), vecOriginLocal = local_player->GetVecOrigin();

	char dist_to[32]; int height = (pos.y - top.y), width = height / 2;

	sprintf_s(dist_to, "%.0f ft", resolve_distance(vecOriginLocal, vecOrigin));
	RENDER::DrawF(pos.x, SETTINGS::settings.ammo_bool ? pos.y + 12 : pos.y + 8, FONTS::visuals_esp_font, true, true, color, dist_to);
}

void CVisuals::DrawWeapon(SDK::CBaseEntity* entity, CColor color, int index, Vector pos, Vector top)
{
	SDK::player_info_t ent_info; INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	if (!weapon) return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));
	if (!c_baseweapon) return;

	bool is_teammate = local_player->GetTeam() == entity->GetTeam(), distanceThing, distanceThing2;
	if (SETTINGS::settings.ammo_bool) distanceThing = true; else distanceThing = false; if (SETTINGS::settings.ammoteam) distanceThing2 = true; else distanceThing2 = false;
	int height = (pos.y - top.y), width = height / 2, distanceOn = distanceThing ? pos.y + 12 : pos.y + 8, distanceOn2 = distanceThing2 ? pos.y + 12 : pos.y + 8;

	if (c_baseweapon->is_revolver())
		RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "R8 REVOLVER");
	else if (c_baseweapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_USP_SILENCER)
		RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "USP-S");
	else if (c_baseweapon->GetItemDefenitionIndex() == SDK::ItemDefinitionIndex::WEAPON_M4A1_SILENCER)
		RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, "M4A1-S");
	else
		RENDER::DrawF(pos.x, is_teammate ? distanceOn2 : distanceOn, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName) + " (" + std::to_string(c_baseweapon->GetLoadedAmmo()) + ")");
}

void CVisuals::DrawHealth(SDK::CBaseEntity* entity, CColor color, CColor dormant, Vector pos, Vector top)
{
	int enemy_hp = entity->GetHealth(),
		hp_red = 255 - (enemy_hp * 2.55),
		hp_green = enemy_hp * 2.55,
		height = (pos.y - top.y),
		width = height / 2;

	float offset = (height / 4.f) + 5;
	UINT hp = height - (UINT)((height * enemy_hp) / 100);

	RENDER::DrawEmptyRect((pos.x - width / 2) - 6, top.y, (pos.x - width / 2) - 3, top.y + height, dormant);
	RENDER::DrawLine((pos.x - width / 2) - 4, top.y + hp, (pos.x - width / 2) - 4, top.y + height, color);
	RENDER::DrawLine((pos.x - width / 2) - 5, top.y + hp, (pos.x - width / 2) - 5, top.y + height, color);

	if (entity->GetHealth() < 100)
		RENDER::DrawF((pos.x - width / 2) - 4, top.y + hp, FONTS::visuals_esp_font, true, true, main_color, std::to_string(enemy_hp));
}

void CVisuals::DrawAmmo(SDK::CBaseEntity* entity, CColor color, CColor dormant, Vector pos, Vector top)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));
	if (!c_baseweapon) return;

	int height = (pos.y - top.y);

	float offset = (height / 4.f) + 5;
	UINT hp = height - (UINT)((height * 3) / 100);

	auto animLayer = entity->GetAnimOverlay(1);
	if (!animLayer.m_pOwner)
		return;

	auto activity = entity->GetSequenceActivity(animLayer.m_nSequence);

	int iClip = c_baseweapon->GetLoadedAmmo();
	int iClipMax = c_baseweapon->get_full_info()->max_clip;

	float box_w = (float)fabs(height / 2);
	float width;
	if (activity == 967 && animLayer.m_flWeight != 0.f)
	{
		float cycle = animLayer.m_flCycle;
		width = (((box_w * cycle) / 1.f));
	}
	else
		width = (((box_w * iClip) / iClipMax));

	RENDER::DrawFilledRect((pos.x - box_w / 2), top.y + height + 3, (pos.x - box_w / 2) + box_w + 2, top.y + height + 7, dormant); //outline
	RENDER::DrawFilledRect((pos.x - box_w / 2) + 1, top.y + height + 4, (pos.x - box_w / 2) + width + 1, top.y + height + 6, color); //ammo
}

void CVisuals::DrawInfo(SDK::CBaseEntity* entity, CColor color, CColor alt, Vector pos, Vector top)
{
	std::vector<std::pair<std::string, CColor>> stored_info;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (SETTINGS::settings.money_bool && !is_teammate)
		stored_info.push_back(std::pair<std::string, CColor>("$" + std::to_string(entity->GetMoney()), backtrack));
	else if (SETTINGS::settings.moneyteam && is_teammate)
		stored_info.push_back(std::pair<std::string, CColor>("$" + std::to_string(entity->GetMoney()), backtrack));

	if (SETTINGS::settings.info_bool && !is_teammate)
	{
		if (entity->GetArmor() > 0)
			stored_info.push_back(std::pair<std::string, CColor>(entity->GetArmorName(), color));

		if (entity->GetIsScoped())
			stored_info.push_back(std::pair<std::string, CColor>("zoom", alt));
	}
	else if (SETTINGS::settings.flagsteam && is_teammate)
	{
		if (entity->GetArmor() > 0)
			stored_info.push_back(std::pair<std::string, CColor>(entity->GetArmorName(), color));

		if (entity->GetIsScoped())
			stored_info.push_back(std::pair<std::string, CColor>("zoom", alt));
	}

	int height = (pos.y - top.y), width = height / 2, i = 0;
	for (auto Text : stored_info)
	{
		RENDER::DrawF((pos.x + width / 2) + 5, top.y + i, FONTS::visuals_esp_font, false, false, Text.second, Text.first);
		i += 8;
	}
}

void CVisuals::DrawInaccuracy()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex()));
	if (!weapon) return;

	int spread_Col[3] = { SETTINGS::settings.spread_Col.RGBA[0], SETTINGS::settings.spread_Col.RGBA[1], SETTINGS::settings.spread_Col.RGBA[2] };


	int W, H, cW, cH;
	INTERFACES::Engine->GetScreenSize(W, H);
	cW = W / 2; cH = H / 2;

	if (local_player->IsAlive())
	{
		auto accuracy = (weapon->GetInaccuracy() + weapon->GetSpreadCone()) * 500.f;

		float r;
		float alpha, newAlpha;

		for (r = accuracy; r>0; r--)
		{
			if (!weapon->is_grenade() && !weapon->is_knife())


				alpha = r / accuracy;
			newAlpha = pow(alpha, 5);

			RENDER::DrawCircle(cW, cH, r, 60, CColor(spread_Col[0], spread_Col[1], spread_Col[2], newAlpha * 130));
		}
	}
}

void CVisuals::DrawIndicator()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;
	if (local_player->GetHealth() <= 0) return;

	float breaking_lby_fraction = fabs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw())) / 180.f;
	float lby_delta = abs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw()));

	int screen_width, screen_height;
	INTERFACES::Engine->GetScreenSize(screen_width, screen_height);

	int iY = 88;
	if (SETTINGS::settings.overrideenable)
	{
		iY += 22; bool overridekeyenabled;
		if (SETTINGS::settings.overridemethod == 0)
			RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, SETTINGS::settings.overridething ? CColor(0, 255, 0) : CColor(255, 0, 0), "OVERRIDE");
		else if (SETTINGS::settings.overridemethod == 1)
		{
			GetAsyncKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.overridekey)) ?
				RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, CColor(0, 255, 0), "OVERRIDE") :
				RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, CColor(255, 0, 0), "OVERRIDE");
		}
	}

	if (SETTINGS::settings.aa_bool && SETTINGS::settings.lbyenable)
	{
		iY += 22;
		RENDER::DrawF(10, screen_height - iY, FONTS::visuals_lby_font, false, false, CColor((1.f - breaking_lby_fraction) * 255.f, breaking_lby_fraction * 255.f, 0), "LBY");
	}
	if (SETTINGS::settings.rifk_arrow)
	{
		auto client_viewangles = Vector();
		INTERFACES::Engine->GetViewAngles(client_viewangles);
		const auto screen_center = Vector2D(screen_width / 2.f, screen_height / 2.f);

		constexpr auto radius = 80.f;
		auto draw_arrow = [&](float rot, CColor color) -> void
		{
			std::vector<SDK::Vertex_t> vertices;
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot) * radius, screen_center.y + sinf(rot) * radius)));
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot + DEG2RAD(8)) * (radius - 12), screen_center.y + sinf(rot + DEG2RAD(8)) * (radius - 12)))); //25
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot - DEG2RAD(8)) * (radius - 12), screen_center.y + sinf(rot - DEG2RAD(8)) * (radius - 12)))); //25
			RENDER::TexturedPolygon(3, vertices, color);
		};

		static auto alpha = 0.f; static auto plus_or_minus = false;
		if (alpha <= 0.f || alpha >= 255.f) plus_or_minus = !plus_or_minus;
		alpha += plus_or_minus ? (255.f / 7 * 0.015) : -(255.f / 7 * 0.015); alpha = clamp(alpha, 0.f, 255.f);

		auto fake_color = CColor(255, 0, 0, alpha);
		const auto fake_rot = DEG2RAD(client_viewangles.y - GLOBAL::fake_angles.y - 90);
		draw_arrow(fake_rot, fake_color);


		auto real_color = CColor(0, 255, 0, alpha);
		const auto real_rot = DEG2RAD(client_viewangles.y - GLOBAL::real_angles.y - 90);
		draw_arrow(real_rot, real_color);
	}
}

void CVisuals::DrawBulletBeams()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	if (!INTERFACES::Engine->IsInGame() || !local_player) { Impacts.clear(); return; }
	if (Impacts.size() > 30) Impacts.pop_back();

	for (int i = 0; i < Impacts.size(); i++)
	{
		auto current = Impacts.at(i);
		if (!current.pPlayer) continue;
		if (current.pPlayer->GetIsDormant()) continue;

		bool is_local_player = current.pPlayer == local_player;
		bool is_teammate = local_player->GetTeam() == current.pPlayer->GetTeam() && !is_local_player;

		if (current.pPlayer == local_player)
			current.color = SETTINGS::settings.bulletlocal_col;
		else if (current.pPlayer != local_player && !is_teammate)
			current.color = SETTINGS::settings.bulletenemy_col;
		else if (current.pPlayer != local_player && is_teammate)
			current.color = SETTINGS::settings.bulletteam_col;

		SDK::BeamInfo_t beamInfo;
		beamInfo.m_nType = SDK::TE_BEAMPOINTS;
		beamInfo.m_pszModelName = "sprites/purplelaser1.vmt";
		beamInfo.m_nModelIndex = -1;
		beamInfo.m_flHaloScale = 0.0f;
		beamInfo.m_flLife = 3.0f;
		beamInfo.m_flWidth = 2.0f;
		beamInfo.m_flEndWidth = 2.0f;
		beamInfo.m_flFadeLength = 0.0f;
		beamInfo.m_flAmplitude = 2.0f;
		beamInfo.m_flBrightness = 255.f;
		beamInfo.m_flSpeed = 0.2f;
		beamInfo.m_nStartFrame = 0;
		beamInfo.m_flFrameRate = 0.f;
		beamInfo.m_flRed = current.color.RGBA[0];
		beamInfo.m_flGreen = current.color.RGBA[1];
		beamInfo.m_flBlue = current.color.RGBA[2];
		beamInfo.m_nSegments = 2;
		beamInfo.m_bRenderable = true;
		beamInfo.m_nFlags = SDK::FBEAM_ONLYNOISEONCE | SDK::FBEAM_NOTILE | SDK::FBEAM_HALOBEAM;

		beamInfo.m_vecStart = current.pPlayer->GetVecOrigin() + current.pPlayer->GetViewOffset();
		beamInfo.m_vecEnd = current.vecImpactPos;

		auto beam = INTERFACES::ViewRenderBeams->CreateBeamPoints(beamInfo);
		if (beam) INTERFACES::ViewRenderBeams->DrawBeam(beam);

		Impacts.erase(Impacts.begin() + i);
	}
}

void CVisuals::DrawCrosshair()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto crosshair = INTERFACES::cvar->FindVar("crosshair");
	if (SETTINGS::settings.xhair_type == 0)
	{
		crosshair->SetValue("1");
		return;
	}
	else
		crosshair->SetValue("0");

	int W, H, cW, cH;
	INTERFACES::Engine->GetScreenSize(W, H);

	cW = W / 2; cH = H / 2;

	int dX = W / 120.f, dY = H / 120.f;
	int drX, drY;

	if (SETTINGS::settings.xhair_type == 2)
	{
		drX = cW - (int)(dX * (((local_player->GetPunchAngles().y * 2.f) * 0.45f) + local_player->GetPunchAngles().y));
		drY = cH + (int)(dY * (((local_player->GetPunchAngles().x * 2.f) * 0.45f) + local_player->GetPunchAngles().x));
	}
	else
	{
		drX = cW;
		drY = cH;
	}

	INTERFACES::Surface->DrawSetColor(BLACK);
	INTERFACES::Surface->DrawFilledRect(drX - 4, drY - 2, drX - 4 + 8, drY - 2 + 4);
	INTERFACES::Surface->DrawFilledRect(drX - 2, drY - 4, drX - 2 + 4, drY - 4 + 8);

	INTERFACES::Surface->DrawSetColor(WHITE);
	INTERFACES::Surface->DrawFilledRect(drX - 3, drY - 1, drX - 3 + 6, drY - 1 + 2);
	INTERFACES::Surface->DrawFilledRect(drX - 1, drY - 3, drX - 1 + 2, drY - 3 + 6);
}

void CVisuals::DrawFovArrows(SDK::CBaseEntity* entity, CColor color)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;
	if (entity->GetIsDormant()) return;

	Vector screenPos, client_viewangles;
	int screen_width = 0, screen_height = 0;
	float radius = 300.f;

	if (UTILS::IsOnScreen(aimbot->get_hitbox_pos(entity, SDK::HitboxList::HITBOX_HEAD), screenPos)) return;

	INTERFACES::Engine->GetViewAngles(client_viewangles);
	INTERFACES::Engine->GetScreenSize(screen_width, screen_height);

	const auto screen_center = Vector(screen_width / 2.f, screen_height / 2.f, 0);
	const auto rot = DEG2RAD(client_viewangles.y - UTILS::CalcAngle(local_player->GetEyePosition(), aimbot->get_hitbox_pos(entity, SDK::HitboxList::HITBOX_HEAD)).y - 90);

	std::vector<SDK::Vertex_t> vertices;

	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot) * radius, screen_center.y + sinf(rot) * radius)));
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot + DEG2RAD(2)) * (radius - 16), screen_center.y + sinf(rot + DEG2RAD(2)) * (radius - 16))));
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot - DEG2RAD(2)) * (radius - 16), screen_center.y + sinf(rot - DEG2RAD(2)) * (radius - 16))));

	RENDER::TexturedPolygon(3, vertices, color);
}

void CVisuals::ModulateWorld()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	static auto sv_skyname = INTERFACES::cvar->FindVar("sv_skyname");
	sv_skyname->nFlags &= ~FCVAR_CHEAT;

	static auto static_val = 100;
	auto night_val = SETTINGS::settings.daytimevalue;

	static bool world_performed = false, world_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
	{
		if (world_performed)
			world_performed = false;
		return;
	}

	if (world_performed != SETTINGS::settings.wolrd_enabled)
	{
		world_lastsetting = SETTINGS::settings.wolrd_enabled;
		world_performed = false;
	}

	if (!local_player)
	{
		if (world_performed)
			world_performed = false;
		return;
	}

	if (world_lastsetting != SETTINGS::settings.wolrd_enabled)
	{
		world_lastsetting = SETTINGS::settings.wolrd_enabled;
		world_performed = false;
	}


	if (!world_performed)
	{
		if (static_val != night_val)
		{
			static_val = night_val;
			for (auto i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i))
			{
				auto pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);
				if (!pMaterial) continue;

				auto finalnightval = night_val / 100.f;
				if (strstr(pMaterial->GetTextureGroupName(), "World"))
					pMaterial->ColorModulate(finalnightval, finalnightval, finalnightval);
				else if (strstr(pMaterial->GetTextureGroupName(), "SkyBox"))
					night_val == 100.f ? sv_skyname->SetValue("vertigoblue_hdr") : sv_skyname->SetValue("sky_csgo_night02");

			}
		}
	}
}

void CVisuals::ModulateSky()
{
	static bool sky_performed = false, sky_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
	{
		if (sky_performed)
			sky_performed = false;
		return;
	}

	if (sky_performed != SETTINGS::settings.sky_enabled)
	{
		sky_lastsetting = SETTINGS::settings.sky_enabled;
		sky_performed = false;
	}

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!local_player)
	{
		if (sky_performed)
			sky_performed = false;
		return;
	}

	if (sky_lastsetting != SETTINGS::settings.sky_enabled)
	{
		sky_lastsetting = SETTINGS::settings.sky_enabled;
		sky_performed = false;
	}

	if (!sky_performed)
	{
		static auto r_DrawSpecificStaticProp = INTERFACES::cvar->FindVar("r_DrawSpecificStaticProp");
		r_DrawSpecificStaticProp->nFlags &= ~FCVAR_CHEAT;
		r_DrawSpecificStaticProp->SetValue(1);

		static auto sv_skyname = INTERFACES::cvar->FindVar("sv_skyname");
		sv_skyname->nFlags &= ~FCVAR_CHEAT;

		static auto mat_postprocess_enable = INTERFACES::cvar->FindVar("mat_postprocess_enable");
		mat_postprocess_enable->SetValue(0);

		for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i))
		{
			SDK::IMaterial *pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), ("SkyBox")))
			{
				if (SETTINGS::settings.sky_enabled)
				{
					pMaterial->ColorModulate(SETTINGS::settings.skycolor);
				}
				else
				{
					pMaterial->ColorModulate(1, 1, 1);
				}
			}

		}
		sky_performed = true;
	}
}

void CVisuals::DrawHitmarker()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;
     if (local_player->GetHealth() <= 0) return;

	static int lineSize = 6;

	static float alpha = 0;
	float step = 255.f / 0.3f * INTERFACES::Globals->frametime;


	if (GLOBAL::flHurtTime + 0.4f >= INTERFACES::Globals->curtime)
		alpha = 255.f;
	else
		alpha -= step;

	if (alpha > 0) {
		int screenSizeX, screenCenterX;
		int screenSizeY, screenCenterY;
		INTERFACES::Engine->GetScreenSize(screenSizeX, screenSizeY);

		screenCenterX = screenSizeX / 2;
		screenCenterY = screenSizeY / 2;
		CColor col = SETTINGS::settings.awcolor;
		RENDER::DrawLine(screenCenterX - lineSize * 2, screenCenterY - lineSize * 2, screenCenterX - (lineSize), screenCenterY - (lineSize), col);
		RENDER::DrawLine(screenCenterX - lineSize * 2, screenCenterY + lineSize * 2, screenCenterX - (lineSize), screenCenterY + (lineSize), col);
		RENDER::DrawLine(screenCenterX + lineSize * 2, screenCenterY + lineSize * 2, screenCenterX + (lineSize), screenCenterY + (lineSize), col);
		RENDER::DrawLine(screenCenterX + lineSize * 2, screenCenterY - lineSize * 2, screenCenterX + (lineSize), screenCenterY - (lineSize), col);
	}
}

void CVisuals::DrawBorderLines()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex());
	if (!weapon) return;

	int screen_x;
	int screen_y;
	int center_x;
	int center_y;
	INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
	INTERFACES::Engine->GetScreenSize(center_x, center_y);
	center_x /= 2; center_y /= 2;

	if (local_player->GetIsScoped())
	{
		RENDER::DrawLine(0, center_y, screen_x, center_y, CColor(0, 0, 0, 255));
		RENDER::DrawLine(center_x, 0, center_x, screen_y, CColor(0, 0, 0, 255));
	}
}

const std::string currentDateTime()
{

	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y | %d | %X", &tstruct);

	return buf;
}

void CVisuals::DrawMisc()
{
}

void CVisuals::DrawLBYTimer(SDK::CBaseEntity* entity, CColor color, CColor dormant)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	int enemy_hp = entity->GetHealth();
	int hp_red = 255 - (enemy_hp * 2.55);
	int hp_green = enemy_hp * 2.55;
	CColor health_color = CColor(130, 56, 240, flPlayerAlpha[entity->GetIndex()]);

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;


	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (entity->GetVelocity().Length2D() > 1)
		return;

	if (is_teammate)
		return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);

		float offset = (height / 4.f) + 5;
		UINT hp = height - (UINT)((height * 3) / 100);

		static float next_lby_update[65];
		static float last_lby[65];
		if (entity->GetVelocity().Length2D() > 28)
			next_lby_update[entity->GetIndex()] = entity->GetSimTime() + 0.22f;
		else
		{
			if (next_lby_update[entity->GetIndex()] <= entity->GetSimTime())
				next_lby_update[entity->GetIndex()] = entity->GetSimTime() + 1.1f;

		}

		last_lby[entity->GetIndex()] = entity->GetLowerBodyYaw();


		float time_remain_to_update = next_lby_update[entity->GetIndex()] - entity->GetSimTime();
		float box_w = (float)fabs(height / 2);
		float width;
		width = (((time_remain_to_update * (box_w / 1.1f))));


		RENDER::DrawFilledRect((pos.x - box_w / 2), top.y + height + 3, (pos.x - box_w / 2) + box_w + 2, top.y + height + 7, dormant); //outline
		RENDER::DrawFilledRect((pos.x - box_w / 2) + 1, top.y + height + 4, (pos.x - box_w / 2) + width + 1, top.y + height + 6, color); //ammo
	}
}

void setClanTag(const char* tag, const char* name)
{
	static auto pSetClanTag = reinterpret_cast<void(__fastcall*)(const char*, const char*)>(((DWORD)UTILS::FindPattern("engine.dll", (PBYTE)"\x53\x56\x57\x8B\xDA\x8B\xF9\xFF\x15\x00\x00\x00\x00\x6A\x24\x8B\xC8\x8B\x30", "xxxxxxxxx????xxxxxx")));
	pSetClanTag(tag, name);
}
int kek = 0;
int autism = 0;

// uhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh
const char* NoNamersAnim[40] =
{
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
	"DogWare",
};


void CVisuals::GameSense()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player) return;

	static size_t lastTime = 0;
	if (GetTickCount() > lastTime)
	{
		kek++;
		if (kek > 10) {
			autism++; if (autism > 37) autism = 0;
			setClanTag(NoNamersAnim[autism], "DogWare");
			lastTime = GetTickCount() + 500;
		}

		if (kek > 11) kek = 0;
	}
}

CVisuals* visuals = new CVisuals();