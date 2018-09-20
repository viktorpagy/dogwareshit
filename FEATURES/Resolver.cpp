#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../UTILS/render.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/ConVar.h"
#include "../SDK/AnimLayer.h"
#include "../UTILS/qangle.h"
#include "../FEATURES/Aimbot.h"
#include "../SDK/Collideable.h"
#include "../SDK/CBaseAnimState.h"
#include "../FEATURES/Autowall.h"
#include "../FEATURES/Resolver.h"

Vector old_calcangle(Vector dst, Vector src)
{
	Vector angles;

	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}
	return angles;
}

float old_normalize(float Yaw)
{
	if (Yaw > 180)
	{
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180)
	{
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}



void CResolver::record(SDK::CBaseEntity* entity, float new_yaw)
{
	if (entity->GetVelocity().Length2D() > 36) return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));
	if (!c_baseweapon) return;

	auto &info = player_info[entity->GetIndex()];
	if (entity->GetActiveWeaponIndex() && info.last_ammo < c_baseweapon->GetLoadedAmmo()) {
		info.last_ammo = c_baseweapon->GetLoadedAmmo();
		return;
	}

	info.unresolved_yaw.insert(info.unresolved_yaw.begin(), new_yaw);
	if (info.unresolved_yaw.size() > 20) info.unresolved_yaw.pop_back();
	if (info.unresolved_yaw.size() < 2) return;

	auto average_unresolved_yaw = 0;
	for (auto val : info.unresolved_yaw)
		average_unresolved_yaw += val;
	average_unresolved_yaw /= info.unresolved_yaw.size();

	int delta = average_unresolved_yaw - entity->GetLowerBodyYaw();
	auto big_math_delta = abs((((delta + 180) % 360 + 360) % 360 - 180));

	info.lby_deltas.insert(info.lby_deltas.begin(), big_math_delta);
	if (info.lby_deltas.size() > 10) {
		info.lby_deltas.pop_back();
	}
}

void CResolver::UpdateResolveRecord(SDK::CBaseEntity* entity)
{
	const auto previous_record = player_resolve_records[entity->GetIndex()];
	auto& record = player_resolve_records[entity->GetIndex()];

	record.resolved_angles = record.networked_angles;
	record.velocity = entity->GetVelocity();
	record.origin = entity->GetVecOrigin();
	record.lower_body_yaw = entity->GetLowerBodyYaw();
	record.is_dormant = entity->GetIsDormant();

	record.resolve_type = 0;

	record.is_balance_adjust_triggered = false, record.is_balance_adjust_playing = false;
	for (int i = 0; i < 15; i++)
	{
		record.anim_layers[i] = entity->GetAnimOverlay(i);

		if (entity->GetSequenceActivity(record.anim_layers[i].m_nSequence) == SDK::CSGO_ACTS::ACT_CSGO_IDLE_TURN_BALANCEADJUST)
		{
			record.is_balance_adjust_playing = true;

			if (record.anim_layers[i].m_flWeight == 1 || record.anim_layers[i].m_flCycle > previous_record.anim_layers[i].m_flCycle)
				record.last_balance_adjust_trigger_time = UTILS::GetCurtime();
			if (fabs(UTILS::GetCurtime() - record.last_balance_adjust_trigger_time) < 0.5f)
				record.is_balance_adjust_triggered = true;
		}
	}

	if (record.is_dormant)
		record.next_predicted_lby_update = FLT_MAX;

	if (record.lower_body_yaw != previous_record.lower_body_yaw && !record.is_dormant && !previous_record.is_dormant)
		record.did_lby_flick = true;

	const bool is_moving_on_ground = record.velocity.Length2D() > 50 && entity->GetFlags() & FL_ONGROUND;
	if (is_moving_on_ground && record.is_balance_adjust_triggered)
		record.is_fakewalking = true;
	else
		record.is_fakewalking = false;

	if (is_moving_on_ground && !record.is_fakewalking && record.velocity.Length2D() > 1.f && !record.is_dormant)
	{
		record.is_last_moving_lby_valid = true;
		record.is_last_moving_lby_delta_valid = false;
		record.shots_missed_moving_lby = 0;
		record.shots_missed_moving_lby_delta = 0;
		record.last_moving_lby = record.lower_body_yaw + 45;
		record.last_time_moving = UTILS::GetCurtime();
	}
	if (!record.is_dormant && previous_record.is_dormant)
	{
		if ((record.origin - previous_record.origin).Length2D() > 16.f)
			record.is_last_moving_lby_valid = false;
	}
	if (!record.is_last_moving_lby_delta_valid && record.is_last_moving_lby_valid && record.velocity.Length2D() < 20 && fabs(UTILS::GetCurtime() - record.last_time_moving) < 1.0)
	{
		if (record.lower_body_yaw != previous_record.lower_body_yaw)
		{
			record.last_moving_lby_delta = MATH::NormalizeYaw(record.last_moving_lby - record.lower_body_yaw);
			record.is_last_moving_lby_delta_valid = true;
		}
	}

	if (MATH::NormalizePitch(record.networked_angles.x) > 5.f)
		record.last_time_down_pitch = UTILS::GetCurtime();

}

int CResolver::GetResolveTypeIndex(unsigned short resolve_type)
{
	if (resolve_type & RESOLVE_TYPE_OVERRIDE)
		return 0;
	else if (resolve_type & RESOLVE_TYPE_NO_FAKE)
		return 1;
	else if (resolve_type & RESOLVE_TYPE_LBY)
		return 2;
	else if (resolve_type & RESOLVE_TYPE_LBY_UPDATE)
		return 3;
	else if (resolve_type & RESOLVE_TYPE_PREDICTED_LBY_UPDATE)
		return 4;
	else if (resolve_type & RESOLVE_TYPE_LAST_MOVING_LBY)
		return 5;
	else if (resolve_type & RESOLVE_TYPE_NOT_BREAKING_LBY)
		return 6;
	else if (resolve_type & RESOLVE_TYPE_BRUTEFORCE)
		return 7;
	else if (resolve_type & RESOLVE_TYPE_LAST_MOVING_LBY_DELTA)
		return 8;
	else if (resolve_type & RESOLVE_TYPE_ANTI_FREESTANDING)
		return 9;

	return 0;
}

bool CResolver::AntiFreestanding(SDK::CBaseEntity* entity, float& yaw)
{
	const auto freestanding_record = player_resolve_records[entity->GetIndex()].anti_freestanding_record;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return false;

	if (freestanding_record.left_damage >= 20 && freestanding_record.right_damage >= 20)
		return false;

	const float at_target_yaw = UTILS::CalcAngle(local_player->GetVecOrigin(), entity->GetVecOrigin()).y;
	if (freestanding_record.left_damage <= 0 && freestanding_record.right_damage <= 0)
	{
		if (freestanding_record.right_fraction < freestanding_record.left_fraction)
			yaw = at_target_yaw + 125.f;
		else
			yaw = at_target_yaw - 73.f;
	}
	else
	{
		if (freestanding_record.left_damage > freestanding_record.right_damage)
			yaw = at_target_yaw + 130.f;
		else
			yaw = at_target_yaw - 49.f;
	}

	return true;
}

void CResolver::ProcessSnapShots()
{
	if (shot_snapshots.size() <= 0)
		return;

	const auto snapshot = shot_snapshots.front();
	if (fabs(UTILS::GetCurtime() - snapshot.time) > 1.f)
	{

		shot_snapshots.erase(shot_snapshots.begin());
		return;
	}

	const int player_index = snapshot.entity->GetIndex();
	if (snapshot.hitgroup_hit != -1)
	{
		for (int i = 0; i < RESOLVE_TYPE_NUM; i++)
		{
			if (snapshot.resolve_record.resolve_type & (1 << i))
			{
				player_resolve_records[player_index].shots_fired[i]++;
				player_resolve_records[player_index].shots_hit[i]++;
			}
		}


	}
	else if (snapshot.first_processed_time != 0.f && fabs(UTILS::GetCurtime() - snapshot.first_processed_time) > 0.1f)
	{
		for (int i = 0; i < RESOLVE_TYPE_NUM; i++)
		{
			if (snapshot.resolve_record.resolve_type & (1 << i))
				player_resolve_records[player_index].shots_fired[i]++;
		}

		if (snapshot.resolve_record.resolve_type & RESOLVE_TYPE_LAST_MOVING_LBY)
			player_resolve_records[player_index].shots_missed_moving_lby++;

		if (snapshot.resolve_record.resolve_type & RESOLVE_TYPE_LAST_MOVING_LBY_DELTA)
			player_resolve_records[player_index].shots_missed_moving_lby_delta++;

	}
	else
		return;

	shot_snapshots.erase(shot_snapshots.begin());
}

void CResolver::ResolveYawBruteforce(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	auto& resolve_record = player_resolve_records[entity->GetIndex()];
	resolve_record.resolve_type |= RESOLVE_TYPE_BRUTEFORCE;

	const float at_target_yaw = UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y;

	const int shots_missed = resolve_record.shots_fired[GetResolveTypeIndex(resolve_record.resolve_type)] -
		resolve_record.shots_hit[GetResolveTypeIndex(resolve_record.resolve_type)];
	switch (shots_missed % 3)
	{
	case 0:
		resolve_record.resolved_angles.y = UTILS::GetLBYRotatedYaw(entity->GetLowerBodyYaw(), at_target_yaw + 60.f);
		break;
	case 1:
		resolve_record.resolved_angles.y = at_target_yaw + 140.f;
		break;
	case 2:
		resolve_record.resolved_angles.y = at_target_yaw - 75.f;
		break;
	}
}

static void nospread_resolve(SDK::CBaseEntity* player, int entID)
{
	if (SETTINGS::settings.nospread);
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float atTargetAngle = UTILS::CalcAngle(local_player->GetHealth() <= 0 ? local_player->GetVecOrigin() : local_position, player->GetVecOrigin()).y;
	Vector velocityAngle;
	MATH::VectorAngles(player->GetVelocity(), velocityAngle);

	float primaryBaseAngle = player->GetLowerBodyYaw();
	float secondaryBaseAngle = velocityAngle.y;

	switch ((shots_missed[entID]) % 10)
	{
	case 0:
		player->EasyEyeAngles()->pitch = atTargetAngle + -90.f;
		player->EasyEyeAngles()->yaw = atTargetAngle + 180.f;
		break;
	case 1:
		player->EasyEyeAngles()->pitch = velocityAngle.y + -120.f;
		player->EasyEyeAngles()->yaw = velocityAngle.y + 180.f;
		break;
	case 2:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -150.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle;
		break;
	case 3:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -180.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	case 4:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -90.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 5:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -120.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 130.f;
		break;
	case 6:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -150.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 180.f;
		break;
	case 7:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -180.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle;
		break;
	case 8:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -90.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 40.f;
		break;
	case 9:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -120.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 90.f;
		break;
	case 10:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -150.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 130.f;
		break;
	case 11:
		player->EasyEyeAngles()->pitch = secondaryBaseAngle + -180.f;
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 70.f;
		break;
	case 12:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -90.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 13:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -100.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 135.f;
		break;
	case 14:
		player->EasyEyeAngles()->pitch = primaryBaseAngle + -133.f;
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	}
}

void CResolver::Nospread(SDK::CBaseEntity* entity) {

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
}

void CResolver::resolve(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!entity) return;
	if (!local_player) return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;
	if (is_local_player) return;
	if (is_teammate) return;
	if (entity->GetHealth() <= 0) return;
	if (local_player->GetHealth() <= 0) return;

	if ((SETTINGS::settings.overridemethod == 1 && GetAsyncKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.overridekey))) || (SETTINGS::settings.overridemethod == 0 && SETTINGS::settings.overridething))
	{
		Vector viewangles; INTERFACES::Engine->GetViewAngles(viewangles);
		auto at_target_yaw = UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y;

		auto delta = MATH::NormalizeYaw(viewangles.y - at_target_yaw);
		auto rightDelta = Vector(entity->GetEyeAngles().x, at_target_yaw + 90, entity->GetEyeAngles().z);
		auto leftDelta = Vector(entity->GetEyeAngles().x, at_target_yaw - 90, entity->GetEyeAngles().z);

		if (delta > 0)
			entity->SetEyeAngles(rightDelta);
		else
			entity->SetEyeAngles(leftDelta);
		return;
	}

	auto &info = player_info[entity->GetIndex()];
	float fl_lby = entity->GetLowerBodyYaw();

	info.lby = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw(), 0.f);
	info.inverse = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 180.f, 0.f);
	info.last_lby = Vector(entity->GetEyeAngles().x, info.last_moving_lby, 0.f);
	info.inverse_left = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 115.f, 0.f);
	info.inverse_right = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() - 115.f, 0.f);

	info.back = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 180.f, 0.f);
	info.right = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 70.f, 0.f);
	info.left = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y - 70.f, 0.f);

	info.backtrack = Vector(entity->GetEyeAngles().x, lby_to_back[entity->GetIndex()], 0.f);

	shots_missed[entity->GetIndex()] = shots_fired[entity->GetIndex()] - shots_hit[entity->GetIndex()];



	if (SETTINGS::settings.fakefix_bool) info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND && !info.could_be_slowmo;
	else info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND;
	auto& resolve_record = player_resolve_records[entity->GetIndex()];
	info.is_jumping = !entity->GetFlags() & FL_ONGROUND;
	info.could_be_slowmo = entity->GetVelocity().Length2D() > 6 && entity->GetVelocity().Length2D() < 36 && !info.is_crouching;
	info.is_crouching = entity->GetFlags() & FL_DUCKING;
	update_time[entity->GetIndex()] = info.next_lby_update_time;

	static float old_simtime[65];
	if (entity->GetSimTime() != old_simtime[entity->GetIndex()])
	{
		using_fake_angles[entity->GetIndex()] = entity->GetSimTime() - old_simtime[entity->GetIndex()] == INTERFACES::Globals->interval_per_tick;
		old_simtime[entity->GetIndex()] = entity->GetSimTime();
	}

	if (!using_fake_angles[entity->GetIndex()])
	{
		if (backtrack_tick[entity->GetIndex()])
		{
			resolve_type[entity->GetIndex()] = 7;
		}

		else if (AntiFreestanding(entity, resolve_record.resolved_angles.y))
		{
			resolve_record.resolve_type |= RESOLVE_TYPE_ANTI_FREESTANDING;
		}
		else if (resolve_record.is_last_moving_lby_valid && resolve_record.shots_missed_moving_lby < 1)
		{
			resolve_record.resolved_angles.y = resolve_record.last_moving_lby;
			resolve_record.resolve_type |= RESOLVE_TYPE_LAST_MOVING_LBY;
		}

		for (int i = 0; i < 64; i++)
		{
			auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
			if (!entity || entity->GetHealth() <= 0 || entity->GetTeam() == local_player->GetTeam())
				continue;
			UpdateResolveRecord(entity);
			if (entity->GetIsDormant())
				continue;
		}

		ProcessSnapShots();

		if (info.stored_lby != entity->GetLowerBodyYaw())
		{
			entity->SetEyeAngles(info.lby);
			info.stored_lby = entity->GetLowerBodyYaw();
			resolve_type[entity->GetIndex()] = 3;
		}
		else if (info.is_moving)
		{
			entity->SetEyeAngles(info.lby);
			info.last_moving_lby = entity->GetLowerBodyYaw();
			info.stored_missed = shots_missed[entity->GetIndex()];
			resolve_type[entity->GetIndex()] = 1;
		}
		else
		{
			if (shots_missed[entity->GetIndex()] > info.stored_missed)
			{
				resolve_type[entity->GetIndex()] = 4;
				switch (shots_missed[entity->GetIndex()] % 4)
				{
				case 0: entity->SetEyeAngles(info.inverse); break;
				case 1: entity->SetEyeAngles(info.left); break;
				case 2: entity->SetEyeAngles(info.back); break;
				case 3: entity->SetEyeAngles(info.right); break;
				}
			}
			else
			{
				resolve_type[entity->GetIndex()] = 2;
				entity->SetEyeAngles(info.last_lby);
			}
		}
	}
	else
	{
		entity->SetEyeAngles(info.lby);
		resolve_type[entity->GetIndex()] = 1;
	}
}

CResolver* resolver = new CResolver();

//Resolvy.Us Resolver Below

Vector old_calcangle2(Vector dst, Vector src)
{
	Vector angles;

	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}

	return angles;
}

float old_normalize2(float Yaw)
{
	if (Yaw > 180)
	{
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180)
	{
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}

float curtime(SDK::CUserCmd* ucmd) {
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	int g_tick = 0;
	SDK::CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * INTERFACES::Globals->interval_per_tick;
	return curtime;
}

bool find_layer(SDK::CBaseEntity* entity, int act, SDK::CAnimationLayer *set)
{
	for (int i = 0; i < 13; i++)
	{
		SDK::CAnimationLayer layer = entity->GetAnimOverlay(i);
		const int activity = entity->GetSequenceActivity(layer.m_nSequence);
		if (activity == act) {
			*set = layer;
			return true;
		}
	}
	return false;
}

void CResolver::record2(SDK::CBaseEntity* entity, float new_yaw)
{
	if (entity->GetVelocity().Length2D() > 36)
		return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	auto &info = player_info[entity->GetIndex()];

	if (entity->GetActiveWeaponIndex() && info.last_ammo < c_baseweapon->GetLoadedAmmo()) {
		info.last_ammo = c_baseweapon->GetLoadedAmmo();
		return;
	}

	info.unresolved_yaw.insert(info.unresolved_yaw.begin(), new_yaw);
	if (info.unresolved_yaw.size() > 20) {
		info.unresolved_yaw.pop_back();
	}

	if (info.unresolved_yaw.size() < 2)
		return;

	auto average_unresolved_yaw = 0;
	for (auto val : info.unresolved_yaw)
		average_unresolved_yaw += val;
	average_unresolved_yaw /= info.unresolved_yaw.size();

	int delta = average_unresolved_yaw - entity->GetLowerBodyYaw();
	auto big_math_delta = abs((((delta + 180) % 360 + 360) % 360 - 180));

	info.lby_deltas.insert(info.lby_deltas.begin(), big_math_delta);
	if (info.lby_deltas.size() > 10) {
		info.lby_deltas.pop_back();
	}
}

typedef void(__cdecl* MsgFn)(const char* msg, va_list);
void hMsg(const char* msg, ...)
{
	if (msg == nullptr)
		return;
	static MsgFn fn = (MsgFn)GetProcAddress(GetModuleHandle("tier0.dll"), "Msg");
	char buffer[989];
	va_list list;
	va_start(list, msg);
	vsprintf(buffer, msg, list);
	va_end(list);
	fn(buffer, list);
}

static void nospread_resolve2(SDK::CBaseEntity* player, int entID)
{

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float atTargetAngle = UTILS::CalcAngle(local_player->GetHealth() <= 0 ? local_player->GetVecOrigin() : local_position, player->GetVecOrigin()).y;
	Vector velocityAngle;
	MATH::VectorAngles(player->GetVelocity(), velocityAngle);

	float primaryBaseAngle = player->GetLowerBodyYaw();
	float secondaryBaseAngle = velocityAngle.y;

	switch ((shots_missed[entID]) % 15)
	{
	case 0:
		player->EasyEyeAngles()->yaw = atTargetAngle + 180.f;
		break;
	case 1:
		player->EasyEyeAngles()->yaw = velocityAngle.y + 180.f;
		break;
	case 2:
		player->EasyEyeAngles()->yaw = primaryBaseAngle;
		break;
	case 3:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 45.f;
		break;
	case 4:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 90.f;
		break;
	case 5:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 130.f;
		break;
	case 6:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 180.f;
		break;
	case 7:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle;
		break;
	case 8:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 40.f;
		break;
	case 9:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 90.f;
		break;
	case 10:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 130.f;
		break;
	case 11:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 70.f;
		break;
	case 12:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 13:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 135.f;
		break;
	case 14:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	}
}


void CResolver::resolve2(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	if (entity->GetHealth() <= 0)
		return;

	auto &info = player_info[entity->GetIndex()];

	float fl_lby = entity->GetLowerBodyYaw();

	info.lby = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw(), 0.f);
	info.inverse = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 180.f, 0.f);
	info.last_lby = Vector(entity->GetEyeAngles().x, info.last_moving_lby, 0.f);
	info.inverse_left = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() + 115.f, 0.f);
	info.inverse_right = Vector(entity->GetEyeAngles().x, entity->GetLowerBodyYaw() - 115.f, 0.f);

	info.back = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 180.f, 0.f);
	info.right = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y + 70.f, 0.f);
	info.left = Vector(entity->GetEyeAngles().x, UTILS::CalcAngle(entity->GetVecOrigin(), local_player->GetVecOrigin()).y - 70.f, 0.f);

	info.backtrack = Vector(entity->GetEyeAngles().x, lby_to_back[entity->GetIndex()], 0.f);

	shots_missed[entity->GetIndex()] = shots_fired[entity->GetIndex()] - shots_hit[entity->GetIndex()];


	if (SETTINGS::settings.fakefix_bool)
		info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND && !info.could_be_slowmo;
	else
		info.is_moving = entity->GetVelocity().Length2D() > 0.1 && entity->GetFlags() & FL_ONGROUND;
	info.is_jumping = !entity->GetFlags() & FL_ONGROUND;
	info.could_be_slowmo = entity->GetVelocity().Length2D() > 6 && entity->GetVelocity().Length2D() < 36 && !info.is_crouching;
	info.is_crouching = entity->GetFlags() & FL_DUCKING;
	update_time[entity->GetIndex()] = info.next_lby_update_time;

	static float old_simtime[65];
	if (entity->GetSimTime() != old_simtime[entity->GetIndex()])
	{
		using_fake_angles[entity->GetIndex()] = entity->GetSimTime() - old_simtime[entity->GetIndex()] == INTERFACES::Globals->interval_per_tick;
		old_simtime[entity->GetIndex()] = entity->GetSimTime();
	}

	if (!using_fake_angles[entity->GetIndex()])
	{
		if (backtrack_tick[entity->GetIndex()])
		{
			resolve_type[entity->GetIndex()] = 7;
			entity->SetEyeAngles(info.backtrack);
		}
		else if (info.stored_lby != entity->GetLowerBodyYaw())
		{
			entity->SetEyeAngles(info.lby);
			info.stored_lby = entity->GetLowerBodyYaw();
			resolve_type[entity->GetIndex()] = 3;
		}
		else if (info.is_moving)
		{
			entity->SetEyeAngles(info.lby);
			info.last_moving_lby = entity->GetLowerBodyYaw();
			info.stored_missed = shots_missed[entity->GetIndex()];
			resolve_type[entity->GetIndex()] = 1;
		}
		else
		{
			if (shots_missed[entity->GetIndex()] > info.stored_missed)
			{
				resolve_type[entity->GetIndex()] = 4;
				switch (shots_missed[entity->GetIndex()] % 4)
				{
				case 0: entity->SetEyeAngles(info.inverse); break;
				case 1: entity->SetEyeAngles(info.left); break;
				case 2: entity->SetEyeAngles(info.back); break;
				case 3: entity->SetEyeAngles(info.right); break;
				}
			}
			else
			{
				entity->SetEyeAngles(info.last_lby);
				resolve_type[entity->GetIndex()] = 5;
			}
		}
	}
}

CResolver* resolver2 = new CResolver();

//EGGHack Resolver Below

Vector old_calcangle3(Vector dst, Vector src)
{
	Vector angles;

	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}

	return angles;
}

float old_normalize3(float Yaw)
{
	if (Yaw > 180)
	{
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180)
	{
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}

float curtime3(SDK::CUserCmd* ucmd) {
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return 0;

	int g_tick = 0;
	SDK::CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * INTERFACES::Globals->interval_per_tick;
	return curtime;
}

bool find_layer3(SDK::CBaseEntity* entity, int act, SDK::CAnimationLayer *set)
{
	for (int i = 0; i < 13; i++)
	{
		SDK::CAnimationLayer layer = entity->GetAnimOverlay(i);
		const int activity = entity->GetSequenceActivity(layer.m_nSequence);
		if (activity == act) {
			*set = layer;
			return true;
		}
	}
	return false;
}

void CResolver::record3(SDK::CBaseEntity* entity, float new_yaw)
{
	if (entity->GetVelocity().Length2D() > 36)
		return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	auto &info = player_info[entity->GetIndex()];

	if (entity->GetActiveWeaponIndex() && info.last_ammo < c_baseweapon->GetLoadedAmmo()) {
		info.last_ammo = c_baseweapon->GetLoadedAmmo();
		return;
	}

	info.unresolved_yaw.insert(info.unresolved_yaw.begin(), new_yaw);
	if (info.unresolved_yaw.size() > 20) {
		info.unresolved_yaw.pop_back();
	}

	if (info.unresolved_yaw.size() < 2)
		return;

	auto average_unresolved_yaw = 0;
	for (auto val : info.unresolved_yaw)
		average_unresolved_yaw += val;
	average_unresolved_yaw /= info.unresolved_yaw.size();

	int delta = average_unresolved_yaw - entity->GetLowerBodyYaw();
	auto big_math_delta = abs((((delta + 180) % 360 + 360) % 360 - 180));

	info.lby_deltas.insert(info.lby_deltas.begin(), big_math_delta);
	if (info.lby_deltas.size() > 10) {
		info.lby_deltas.pop_back();
	}
}

static void nospread_resolve3(SDK::CBaseEntity* player, int entID)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	float atTargetAngle = UTILS::CalcAngle(local_player->GetHealth() <= 0 ? local_player->GetVecOrigin() : local_position, player->GetVecOrigin()).y;
	Vector velocityAngle;
	MATH::VectorAngles(player->GetVelocity(), velocityAngle);

	float primaryBaseAngle = player->GetLowerBodyYaw();
	float secondaryBaseAngle = velocityAngle.y;

	switch ((shots_missed[entID]) % 15)
	{
	case 0:
		player->EasyEyeAngles()->yaw = atTargetAngle + 180.f;
		break;
	case 1:
		player->EasyEyeAngles()->yaw = velocityAngle.y + 180.f;
		break;
	case 2:
		player->EasyEyeAngles()->yaw = primaryBaseAngle;
		break;
	case 3:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 45.f;
		break;
	case 4:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 90.f;
		break;
	case 5:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 130.f;
		break;
	case 6:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 180.f;
		break;
	case 7:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle;
		break;
	case 8:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 40.f;
		break;
	case 9:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 90.f;
		break;
	case 10:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 130.f;
		break;
	case 11:
		player->EasyEyeAngles()->yaw = secondaryBaseAngle - 70.f;
		break;
	case 12:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 45.f;
		break;
	case 13:
		player->EasyEyeAngles()->yaw = primaryBaseAngle + 135.f;
		break;
	case 14:
		player->EasyEyeAngles()->yaw = primaryBaseAngle - 90.f;
		break;
	}
}

void CResolver::resolve3(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!entity)
		return;

	if (!local_player)
		return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	if (entity->GetHealth() <= 0)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	static float old_simtime[65];
	if (entity->GetSimTime() != old_simtime[entity->GetIndex()])
	{
		using_fake_angles[entity->GetIndex()] = entity->GetSimTime() - old_simtime[entity->GetIndex()] != INTERFACES::Globals->interval_per_tick;
		old_simtime[entity->GetIndex()] = entity->GetSimTime();
	}

	auto pick_best = [](float primary, float secondary, float defined, bool accurate) -> float
	{
		if (accurate)
		{
			if (MATH::YawDistance(primary, defined) <= 50)
				return primary;
			else if (MATH::YawDistance(secondary, defined) <= 50)
				return secondary;
			else
				return defined;
		}
		else
		{
			if (MATH::YawDistance(primary, defined) <= 80)
				return primary;
			else if (MATH::YawDistance(secondary, defined) <= 80)
				return secondary;
			else
				return defined;
		}
	};

	if (using_fake_angles[entity->GetIndex()])
	{
		static auto nospread = INTERFACES::cvar->FindVar("weapon_accuracy_nospread")->GetBool();

		if (nospread)
		{
		}
		else
		{
		}
	}
}

CResolver* resolver3 = new CResolver();