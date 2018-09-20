/*#include "../includes.h"

#include "interfaces.h"
#include "../SDK/IClient.h"
#include "../SDK/RecvData.h"

#include "NetvarHookManager.h"

namespace UTILS
{
CNetvarHookManager netvar_hook_manager;
void CNetvarHookManager::Hook(std::string table_to_hook, std::string prop_to_hook, NetvarHookCallback callback)
{
auto client_class = INTERFACES::Client->GetAllClasses();

// go through all classes
while (client_class)
{
std::string table_name = client_class->m_pRecvTable->m_pNetTableName;

if (table_name == table_to_hook)
{
for (int i = 0; i < client_class->m_pRecvTable->m_nProps; i++)
{
auto& prop = client_class->m_pRecvTable->m_pProps[i];
std::string prop_name = prop.m_pVarName;

if (prop_name == prop_to_hook)
{
prop.m_ProxyFn = callback;
}
}
}

client_class = client_class->m_pNext; // go to the next class
}
}

uintptr_t CNetvarHookManager::GetOffset(std::string table, std::string prop)
{
auto client_class = INTERFACES::Client->GetAllClasses();

// go through all classes
while (client_class)
{
auto return_value = RecursivelyFindOffset(client_class->m_pRecvTable, table, prop);
if (return_value)
return return_value;

client_class = client_class->m_pNext; // go to the next class
}

LOG("** Failed to get offset - " + prop);
return 0;
}

uintptr_t CNetvarHookManager::RecursivelyFindOffset(SDK::RecvTable* table, std::string table_name, std::string prop_name)
{
for (int i = 0; i < table->m_nProps; i++)
{
if (table_name == table->m_pNetTableName && prop_name == table->m_pProps[i].m_pVarName)
{
LOG(std::string("Successfully found offset ") + table->m_pNetTableName + " " + table->m_pProps[i].m_pVarName);
return table->m_pProps[i].m_Offset;
}

if (!table->m_pProps[i].m_pDataTable)
continue;

auto return_value = RecursivelyFindOffset(table->m_pProps[i].m_pDataTable, table_name, prop_name);
if (return_value)
return return_value;
}

return 0;
}
}*/
#include "../includes.h"

#include "interfaces.h"
#include "../SDK/IClient.h"
#include "../SDK/RecvData.h"

#include "NetvarHookManager.h"

#include <string.h>



namespace UTILS
{
	CNetvarHookManager netvar_hook_manager;
	void CNetvarHookManager::Hook(std::string table_to_hook, std::string prop_to_hook, NetvarHookCallback callback)
	{
		auto client_class = INTERFACES::Client->GetAllClasses();

		// go through all classes
		while (client_class)
		{
			std::string table_name = client_class->m_pRecvTable->m_pNetTableName;

			if (table_name == table_to_hook)
			{
				for (int i = 0; i < client_class->m_pRecvTable->m_nProps; i++)
				{
					auto& prop = client_class->m_pRecvTable->m_pProps[i];
					std::string prop_name = prop.m_pVarName;

					if (prop_name == prop_to_hook)
					{
						prop.m_ProxyFn = callback;
					}
				}
			}

			client_class = client_class->m_pNext; // go to the next class
		}
	}
	int CNetvarHookManager::GetOffset(const char* tableName, const char* propName)
	{
		int offset = Get_Prop(tableName, propName);
		if (!offset)
		{
			return 0;
		}
		return offset;
	}
	int CNetvarHookManager::Get_Prop(const char* tableName, const char* propName, SDK::RecvProp** prop)
	{
		SDK::RecvTable* recvTable = GetTable(tableName);
		if (!recvTable)
			return 0;

		int offset = Get_Prop(recvTable, propName, prop);
		if (!offset)
			return 0;

		return offset;
	}
	void CNetvarHookManager::Initialize()
	{
		m_tables.clear();

		SDK::ClientClass* clientClass = INTERFACES::Client->GetAllClasses();
		if (!clientClass)
			return;

		while (clientClass)
		{
			SDK::RecvTable* recvTable = clientClass->m_pRecvTable;
			m_tables.push_back(recvTable);

			clientClass = clientClass->m_pNext;
		}
	}
	SDK::RecvTable* CNetvarHookManager::GetTable(const char* tableName)
	{
		if (m_tables.empty())
			return 0;

		for each(SDK::RecvTable* table in m_tables)
		{
			if (!table)
				continue;

			if (_stricmp(table->m_pNetTableName, tableName) == 0)
				return table;
		}

		return 0;
	}
	int CNetvarHookManager::Get_Prop(SDK::RecvTable* recvTable, const char* propName, SDK::RecvProp** prop)
	{
		int extraOffset = 0;
		for (int i = 0; i < recvTable->m_nProps; ++i)
		{
			SDK::RecvProp* recvProp = &recvTable->m_pProps[i];
			SDK::RecvTable* child = recvProp->m_pDataTable;

			if (child && (child->m_nProps > 0))
			{
				int tmp = Get_Prop(child, propName, prop);
				if (tmp)
					extraOffset += (recvProp->m_Offset + tmp);
			}

			if (_stricmp((const char*)recvProp->m_pVarName, propName))
				continue;

			if (prop)
				*prop = recvProp;

			return (recvProp->m_Offset + extraOffset);
		}

		return extraOffset;
	}

}