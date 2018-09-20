#pragma once
namespace SDK
{
	class CUserCmd;
	class CBaseEntity;
}

class CAntiAim
{
public:
	void do_antiaim(SDK::CUserCmd* cmd);
	float TimeUntilNextLBYUpdate()
	{
		return m_next_lby_update_time - UTILS::GetCurtime();
	}
	void fix_movement(SDK::CUserCmd* cmd);
	Vector fix_movement(SDK::CUserCmd* cmd, SDK::CUserCmd orignal);
private:
	void backwards(SDK::CUserCmd* cmd);
	float m_next_lby_update_time = 0.f, m_last_move_time = 0.f, m_last_attempted_lby = 0.f;
	bool m_will_lby_update = false;

	void legit(SDK::CUserCmd* cmd);
	void sidespin(SDK::CUserCmd * cmd);
	void freestand(SDK::CUserCmd * cmd);
	void sideways(SDK::CUserCmd* cmd);
	void lowerbody(SDK::CUserCmd* cmd);
	void backjitter(SDK::CUserCmd* cmd);
	void lowerbody_pysen(SDK::CUserCmd* cmd);

};

extern CAntiAim* antiaim;