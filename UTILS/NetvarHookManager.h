/*#pragma once

namespace SDK
{
class CRecvProxyData;
struct RecvTable;
}
namespace UTILS
{
typedef void(*NetvarHookCallback)(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut);

class CNetvarHookManager
{
public:
void Hook(std::string table, std::string prop, NetvarHookCallback callback);
uintptr_t GetOffset(std::string table, std::string prop);
uintptr_t RecursivelyFindOffset(SDK::RecvTable * table, std::string table_name, std::string prop_name);
;
private:
};
extern CNetvarHookManager netvar_hook_manager;
}*/
#pragma once

namespace SDK
{
	class CRecvProxyData;
	struct RecvTable;
	struct RecvProp;
}
namespace UTILS
{
	typedef void(*NetvarHookCallback)(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut);

	class CNetvarHookManager
	{
	public:
		void Initialize();
		void Hook(std::string table, std::string prop, NetvarHookCallback callback);
		int GetOffset(const char* tableName, const char* propName);
	private:
		int Get_Prop(const char* tableName, const char* propName, SDK::RecvProp** prop = 0);
		int Get_Prop(SDK::RecvTable* recvTable, const char* propName, SDK::RecvProp** prop = 0);
		SDK::RecvTable* GetTable(const char* tableName);
		std::vector< SDK::RecvTable* > m_tables;
	}; extern CNetvarHookManager netvar_hook_manager;
}