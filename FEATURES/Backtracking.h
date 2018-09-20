#pragma once

#define BACKTRACKING_ENABLED

#define MAX_RECORDS 64
#define ACCELERATION_CALC_TIME 0.3f
#define ROTATION_CALC_TIME 0.5f
#define DIRECTION_CALC_TIME 0.5f
#define FLAG_CALC_TIME 0.9f

#define MAXSTUDIOBONES 128		// total bones actually used
#define MAX_PLAYERS 65

#include <deque>
#include <array>

namespace SDK
{
	class CUserCmd;
	class CBaseEntity;
	class ConVar;
}

struct legit_backtrackdata
{
	float simtime;
	Vector hitboxPos;
};

class VarMapEntry_t
{
public:
	unsigned short type;
	unsigned short m_bNeedsToInterpolate; // Set to false when this var doesn't
										  // need Interpolate() called on it anymore.
	void *data;
	void *watcher;
};

struct VarMapping_t
{
	VarMapping_t()
	{
		m_nInterpolatedEntries = 0;
	}

	VarMapEntry_t* m_Entries;
	int m_nInterpolatedEntries;
	float m_lastInterpolationTime;
};

class CBacktrack
{
public:
	void backtrack_player(SDK::CUserCmd * cmd);
	void DisableInterpolation(SDK::CBaseEntity* pEntity)
	{
		VarMapping_t* map = GetVarMap(pEntity);
		if (!map) return;
		for (int i = 0; i < map->m_nInterpolatedEntries; i++)
		{
			VarMapEntry_t *e = &map->m_Entries[i];
			e->m_bNeedsToInterpolate = false;
		}
	}
private:
	VarMapping_t * GetVarMap(void* pBaseEntity)
	{
		return reinterpret_cast<VarMapping_t*>((DWORD)pBaseEntity + 0x24);
	}
	int latest_tick;
	Vector angle_vector(Vector meme);
	float point_to_line(Vector Point, Vector LineOrigin, Vector Dir);
	bool IsValid(SDK::CBaseEntity * player);
};

extern legit_backtrackdata headPositions[64][12];
extern CBacktrack* backtracking;
