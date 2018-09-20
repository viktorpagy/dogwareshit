#include "Skinchanger.h"
#include "recv.h"


typedef void(*RecvVarProxyFn)(const SDK::CRecvProxyData* pData, void* pStruct, void* pOut);

#define SEQUENCE_DEFAULT_DRAW 0
#define SEQUENCE_DEFAULT_IDLE1 1
#define SEQUENCE_DEFAULT_IDLE2 2
#define SEQUENCE_DEFAULT_LIGHT_MISS1 3
#define SEQUENCE_DEFAULT_LIGHT_MISS2 4
#define SEQUENCE_DEFAULT_HEAVY_MISS1 9
#define SEQUENCE_DEFAULT_HEAVY_HIT1 10
#define SEQUENCE_DEFAULT_HEAVY_BACKSTAB 11
#define SEQUENCE_DEFAULT_LOOKAT01 12

#define SEQUENCE_BUTTERFLY_DRAW 0
#define SEQUENCE_BUTTERFLY_DRAW2 1
#define SEQUENCE_BUTTERFLY_LOOKAT01 13
#define SEQUENCE_BUTTERFLY_LOOKAT03 15

#define SEQUENCE_FALCHION_IDLE1 1
#define SEQUENCE_FALCHION_HEAVY_MISS1 8
#define SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP 9
#define SEQUENCE_FALCHION_LOOKAT01 12
#define SEQUENCE_FALCHION_LOOKAT02 13

#define SEQUENCE_DAGGERS_IDLE1 1
#define SEQUENCE_DAGGERS_LIGHT_MISS1 2
#define SEQUENCE_DAGGERS_LIGHT_MISS5 6
#define SEQUENCE_DAGGERS_HEAVY_MISS2 11
#define SEQUENCE_DAGGERS_HEAVY_MISS1 12

#define SEQUENCE_BOWIE_IDLE1 1
using namespace std;
// Helper function to generate a random sequence number.
inline int RandomSequence(int low, int high) {
	return (rand() % (high - low + 1) + low);
}

#define	LIFE_ALIVE 0

#define RandomInt(nMin, nMax) (rand() % (nMax - nMin + 1) + nMin);



RecvVarProxyFn fnSequenceProxyFn = nullptr;

RecvVarProxyFn oRecvnModelIndex;
void Hooked_RecvProxy_Viewmodel(SDK::CRecvProxyData *pData, void *pStruct, void *pOut)
{
	// Get the knife view model id's
	int default_t = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_default_t.mdl");
	int default_ct = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_default_ct.mdl");
	int iBayonet = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_bayonet.mdl");
	int iButterfly = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_butterfly.mdl");
	int iFlip = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_flip.mdl");
	int iGut = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_gut.mdl");
	int iKarambit = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_karam.mdl");
	int iM9Bayonet = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_m9_bay.mdl");
	int iHuntsman = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_tactical.mdl");
	int iFalchion = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_falchion_advanced.mdl");
	int iDagger = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_push.mdl");
	int iBowie = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_survival_bowie.mdl");
	int iGunGame = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_gg.mdl");

	int Navaja = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_gypsy_jackknife.mdl");
	int Stiletto = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_stiletto.mdl");
	int Ursus = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_ursus.mdl");
	int Talon = INTERFACES::ModelInfo->GetModelIndex("models/weapons/v_knife_widowmaker.mdl");

	// Get local player (just to stop replacing spectators knifes)
	SDK::CBaseEntity* pLocal = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (SETTINGS::settings.skinchanger_enable && pLocal)
	{
		// If we are alive and holding a default knife(if we already have a knife don't worry about changing)
		if (pLocal->IsAlive() && (
			pData->m_Value.m_Int == default_t ||
			pData->m_Value.m_Int == default_ct ||
			pData->m_Value.m_Int == iBayonet ||
			pData->m_Value.m_Int == iFlip ||
			pData->m_Value.m_Int == iGunGame ||
			pData->m_Value.m_Int == iGut ||
			pData->m_Value.m_Int == iKarambit ||
			pData->m_Value.m_Int == iM9Bayonet ||
			pData->m_Value.m_Int == iHuntsman ||
			pData->m_Value.m_Int == iBowie ||
			pData->m_Value.m_Int == iButterfly ||
			pData->m_Value.m_Int == iFalchion ||
			pData->m_Value.m_Int == iDagger ||
			pData->m_Value.m_Int == Navaja ||
			pData->m_Value.m_Int == Stiletto ||
			pData->m_Value.m_Int == Ursus ||
			pData->m_Value.m_Int == Talon))
		{
			// Set whatever knife we want
			if (SETTINGS::settings.skinchanger_knifemodel == 0)
				pData->m_Value.m_Int = iBayonet;
			else if (SETTINGS::settings.skinchanger_knifemodel == 9)
				pData->m_Value.m_Int = iBowie;
			else if (SETTINGS::settings.skinchanger_knifemodel == 6)
				pData->m_Value.m_Int = iButterfly;
			else if (SETTINGS::settings.skinchanger_knifemodel == 7)
				pData->m_Value.m_Int = iFalchion;
			else if (SETTINGS::settings.skinchanger_knifemodel == 1)
				pData->m_Value.m_Int = iFlip;
			else if (SETTINGS::settings.skinchanger_knifemodel == 2)
				pData->m_Value.m_Int = iGut;
			else if (SETTINGS::settings.skinchanger_knifemodel == 5)
				pData->m_Value.m_Int = iHuntsman;
			else if (SETTINGS::settings.skinchanger_knifemodel == 3)
				pData->m_Value.m_Int = iKarambit;
			else if (SETTINGS::settings.skinchanger_knifemodel == 4)
				pData->m_Value.m_Int = iM9Bayonet;
			else if (SETTINGS::settings.skinchanger_knifemodel == 8)
				pData->m_Value.m_Int = iDagger;
			else if (SETTINGS::settings.skinchanger_knifemodel == 10)
				pData->m_Value.m_Int = Navaja;
			else if (SETTINGS::settings.skinchanger_knifemodel == 11)
				pData->m_Value.m_Int = Stiletto;
			else if (SETTINGS::settings.skinchanger_knifemodel == 12)
				pData->m_Value.m_Int = Ursus;
			else if (SETTINGS::settings.skinchanger_knifemodel == 13)
				pData->m_Value.m_Int = Talon;
		}
	}
	// Carry on the to original proxy
	oRecvnModelIndex(pData, pStruct, pOut);
}





void SetViewModelSequence2(const SDK::CRecvProxyData *pDataConst, void *pStruct, void *pOut)
{

	// Make the incoming data editable.
	SDK::CRecvProxyData* pData = const_cast<SDK::CRecvProxyData*>(pDataConst);

	// Confirm that we are replacing our view model and not someone elses.
	SDK::CBaseViewModel* pViewModel = static_cast<SDK::CBaseViewModel*>(pStruct);

	if (pViewModel)
	{
		SDK::CBaseEntity* pOwner = static_cast<SDK::CBaseEntity*>(INTERFACES::ClientEntityList->GetClientEntity(pViewModel->GetOwner() & 0xFFF));

		// Compare the owner entity of this view model to the local player entity.
		if (pOwner && pOwner->GetIndex() == INTERFACES::Engine->GetLocalPlayer())
		{
			// Get the filename of the current view model.
			void* pModel = INTERFACES::ModelInfo->GetModelvoid(pViewModel->GetModelIndex());
			std::string szModel = INTERFACES::ModelInfo->GetModelNamevoid(pModel);

			// Store the current sequence.
			int m_nSequence = pData->m_Value.m_Int;
			if (szModel == "models/weapons/v_knife_butterfly.mdl")
			{
				// Fix animations for the Butterfly Knife.
				switch (m_nSequence)
				{
				case SEQUENCE_DEFAULT_DRAW:
					m_nSequence = RandomInt(SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2);
					break;
				case SEQUENCE_DEFAULT_LOOKAT01:
					m_nSequence = RandomInt(SEQUENCE_BUTTERFLY_LOOKAT01, SEQUENCE_BUTTERFLY_LOOKAT03);
					break;
				default:
					m_nSequence++;
				}
			}
			else if (szModel == "models/weapons/v_knife_falchion_advanced.mdl")
			{
				// Fix animations for the Falchion Knife.
				switch (m_nSequence)
				{
				case SEQUENCE_DEFAULT_IDLE2:
					m_nSequence = SEQUENCE_FALCHION_IDLE1; break;
				case SEQUENCE_DEFAULT_HEAVY_MISS1:
					m_nSequence = RandomInt(SEQUENCE_FALCHION_HEAVY_MISS1, SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP);
					break;
				case SEQUENCE_DEFAULT_LOOKAT01:
					m_nSequence = RandomInt(SEQUENCE_FALCHION_LOOKAT01, SEQUENCE_FALCHION_LOOKAT02);
					break;
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					break;
				default:
					m_nSequence--;
				}
			}
			else if (szModel == "models/weapons/v_knife_push.mdl")
			{
				// Fix animations for the Shadow Daggers.
				switch (m_nSequence)
				{
				case SEQUENCE_DEFAULT_IDLE2:
					m_nSequence = SEQUENCE_DAGGERS_IDLE1; break;
				case SEQUENCE_DEFAULT_LIGHT_MISS1:
				case SEQUENCE_DEFAULT_LIGHT_MISS2:
					m_nSequence = RandomInt(SEQUENCE_DAGGERS_LIGHT_MISS1, SEQUENCE_DAGGERS_LIGHT_MISS5);
					break;
				case SEQUENCE_DEFAULT_HEAVY_MISS1:
					m_nSequence = RandomInt(SEQUENCE_DAGGERS_HEAVY_MISS2, SEQUENCE_DAGGERS_HEAVY_MISS1);
					break;
				case SEQUENCE_DEFAULT_HEAVY_HIT1:
				case SEQUENCE_DEFAULT_HEAVY_BACKSTAB:
				case SEQUENCE_DEFAULT_LOOKAT01:
					m_nSequence += 3; break;
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					break;
				default:
					m_nSequence += 2;
				}
			}
			else if (szModel == "models/weapons/v_knife_survival_bowie.mdl")
			{
				// Fix animations for the Bowie Knife.
				switch (m_nSequence)
				{
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					break;
				case SEQUENCE_DEFAULT_IDLE2:
					m_nSequence = SEQUENCE_BOWIE_IDLE1;
					break;
				default:
					m_nSequence--;
				}
			}
			else if (szModel == "models/weapons/v_knife_ursus.mdl")
			{
				// Fix animations for the Bowie Knife.
				switch (m_nSequence)
				{
				case SEQUENCE_DEFAULT_DRAW:
					m_nSequence = RandomInt(SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2);
					break;
				case SEQUENCE_DEFAULT_LOOKAT01:
					m_nSequence = RandomInt(SEQUENCE_BUTTERFLY_LOOKAT01, SEQUENCE_BUTTERFLY_LOOKAT03);
					break;
				default:
					m_nSequence++;
					break;
				}
			}
			else if (szModel == "models/weapons/v_knife_stiletto.mdl")
			{
				// Fix animations for the Bowie Knife.
				switch (m_nSequence)
				{
				case SEQUENCE_DEFAULT_LOOKAT01:
					m_nSequence = RandomInt(12, 13);
					break;
				}
			}
			else if (szModel == "models/weapons/v_knife_widowmaker.mdl")
			{
				// Fix animations for the Bowie Knife.
				switch (m_nSequence)
				{
				case SEQUENCE_DEFAULT_LOOKAT01:
					m_nSequence = RandomInt(14, 15);
					break;
				}
			}
			pData->m_Value.m_Int = m_nSequence;
		}
	}
	fnSequenceProxyFn(pData, pStruct, pOut);
}


void AnimationFixHook()
{
	for (SDK::ClientClass* pClass = INTERFACES::Client->GetAllClasses(); pClass; pClass = pClass->m_pNext) {
		if (!strcmp(pClass->m_pNetworkName, "CBaseViewModel")) {
			// Search for the 'm_nModelIndex' property.
			SDK::RecvTable* pClassTable = pClass->m_pRecvTable;

			for (int nIndex = 0; nIndex < pClassTable->m_nProps; nIndex++) {
				SDK::RecvProp* pProp = &pClassTable->m_pProps[nIndex];

				if (!pProp || strcmp(pProp->m_pVarName, "m_nSequence"))
					continue;

				// Store the original proxy function.
				fnSequenceProxyFn = static_cast<RecvVarProxyFn>(pProp->m_ProxyFn);

				// Replace the proxy function with our sequence changer.
				pProp->m_ProxyFn = static_cast<RecvVarProxyFn>(SetViewModelSequence2);

				break;
			}

			break;
		}
	}
}

void NetvarHook()
{
	AnimationFixHook();
	SDK::ClientClass *pClass = INTERFACES::Client->GetAllClasses();
	while (pClass)
	{
		const char *pszName = pClass->m_pRecvTable->m_pNetTableName;
		if (!strcmp(pszName, "DT_SmokeGrenadeProjectile"))
		{
			for (int i = 0; i < pClass->m_pRecvTable->m_nProps; i++)
			{
				SDK::RecvProp *pProp = &(pClass->m_pRecvTable->m_pProps[i]);
				const char *name = pProp->m_pVarName;
				if (!strcmp(name, "m_bDidSmokeEffect"))
				{
					//fnNoSmoke = (RecvVarProxyFn)pProp->m_ProxyFn;
					//pProp->m_ProxyFn = NoSmoke;
				}

			}
		}
		else if (!strcmp(pszName, "DT_BaseViewModel"))
		{
			for (int i = 0; i < pClass->m_pRecvTable->m_nProps; i++)
			{
				SDK::RecvProp *pProp = &(pClass->m_pRecvTable->m_pProps[i]);
				const char *name = pProp->m_pVarName;
				if (!strcmp(name, "m_nModelIndex"))
				{
					oRecvnModelIndex = (RecvVarProxyFn)pProp->m_ProxyFn;
					pProp->m_ProxyFn = Hooked_RecvProxy_Viewmodel;
				}
			}
		}
		pClass = pClass->m_pNext;
	}
}