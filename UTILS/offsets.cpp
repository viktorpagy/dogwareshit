#include "../includes.h"
#include "NetvarHookManager.h"
#include "offsets.h"

namespace OFFSETS
{
	offset16 m_iHealth;
	offset16 m_fFlags;
	offset16 m_vecVelocity;
	offset16 m_flLowerBodyYawTarget;
	offset16 deadflag;
	offset16 m_vecOrigin;
	offset16 m_iTeamNum;
	offset16 m_nTickBase;
	offset16 m_bDormant;
	offset16 animstate;
	offset16 m_Collision;
	offset16 m_angEyeAngles;
	offset16 m_flSimulationTime;
	offset16 m_vecViewOffset;
	offset16 m_dwBoneMatrix;
	offset16 m_aimPunchAngle;
	offset16 m_bGunGameImmunity;
	offset16 m_nForceBone;
	offset16 m_flPoseParameter;
	offset16 m_bClientSideAnimation;
	DWORD dwGlowObjectManager;
	offset16 m_flNextPrimaryAttack;
	offset16 m_flNextAttack;
	offset16 m_hActiveWeapon;
	offset16 m_ArmorValue;
	offset16 m_bHasHelmet;
	offset16 m_iObserverMode;
	offset16 m_flCycle;
	offset16 m_nSequence;
	offset16 m_bIsScoped;
	DWORD m_iAccount;
	offset16 m_iPlayerC4;
	offset16 dwPlayerResource;
	offset16 m_lifeState;
	offset16 m_iItemDefinitionIndex;
	offset16 m_flPostponeFireReadyTime;
	offset16 m_fThrowTime;
	offset16 m_bPinPulled;
	offset16 m_MoveType;
	offset16 m_viewPunchAngle;
	offset16 m_hMyWearables;
	uintptr_t m_nModelIndex;
	uintptr_t m_iViewModelIndex;
	uintptr_t m_hWeaponWorldModel;

	uintptr_t m_iClip1;

	void InitOffsets()
	{
		UTILS::netvar_hook_manager.Initialize();
		m_hMyWearables = UTILS::netvar_hook_manager.GetOffset(("DT_BaseCombatCharacter"), ("m_hMyWearables"));
		m_iHealth = UTILS::netvar_hook_manager.GetOffset(("DT_BasePlayer"), ("m_iHealth"));
		m_fFlags = 0x100;
		m_vecVelocity = 0x110;
		m_flLowerBodyYawTarget = UTILS::netvar_hook_manager.GetOffset(("DT_CSPlayer"), ("m_flLowerBodyYawTarget"));
		deadflag = 0x31C4;
		m_vecOrigin = 0x134;
		m_iTeamNum = 0xF0;
		m_nTickBase = 0x3404;
		m_bDormant = 0xE9;
		animstate = 0x3884;
		m_Collision = 0x318;
		m_angEyeAngles = UTILS::netvar_hook_manager.GetOffset(("DT_CSPlayer"), ("m_angEyeAngles[0]")); // memes
		m_iClip1 = UTILS::netvar_hook_manager.GetOffset(("DT_BaseCombatWeapon"), ("m_iClip1"));
		m_flSimulationTime = 0x264;
		m_vecViewOffset = 0x104;
		m_dwBoneMatrix = 0x2698;
		m_aimPunchAngle = 0x301C;
		m_bGunGameImmunity = 0x38A4;
		m_nForceBone = 0x267C;
		m_flPoseParameter = 0x2764;
		dwGlowObjectManager = 0x4FC0ED8;
		m_flNextPrimaryAttack = 0x3208;
		m_flNextAttack = 0x2D60;
		m_hActiveWeapon = 0x2EE8;
		m_ArmorValue = 0xB24C;
		m_bHasHelmet = 0xB240;
		m_iObserverMode = 0x334C;
		m_bIsScoped = 0x388E;
		m_iAccount = 0x2FB8;
		m_iPlayerC4 = 0x161C;
		dwPlayerResource = 0x2ED1A7C;
		m_iItemDefinitionIndex = 0x2F9A;
		m_lifeState = 0x25B;
		m_flPostponeFireReadyTime = 0x32C4;
		m_fThrowTime = 0x3344;
		m_bPinPulled = 0x3332;
		m_MoveType = 0x258;
		m_viewPunchAngle = 0x3010;
		m_nModelIndex = 0x254;
		m_iViewModelIndex = 0x3210;
		m_hWeaponWorldModel = 0x3224;
	}
}