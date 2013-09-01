#include "TrifleInternals.h"
#include "VersionInfo.h"

IDebugLog gLog("Trifle.log");

extern "C"
{
	bool OBSEPlugin_Query(const OBSEInterface * obse, PluginInfo * info)
	{
		_MESSAGE("Trifle Initializing...");
		
		info->infoVersion =	PluginInfo::kInfoVersion;
		info->name =		"Trifle";
		info->version =		PACKED_SME_VERSION;

		Interfaces::kOBSEPluginHandle = obse->GetPluginHandle();

		if (obse->isEditor)
		{
			return false;
		}
		else
		{
			if (obse->oblivionVersion != OBLIVION_VERSION)
			{
				_MESSAGE("Unsupported runtime version %08X", obse->oblivionVersion);
				return false;
			}
			else if(obse->obseVersion < OBSE_VERSION_INTEGER)
			{
				_ERROR("OBSE version too old (got %08X expected at least %08X)", obse->obseVersion, OBSE_VERSION_INTEGER);
				return false;
			}
		}

		return true;
	}

	bool OBSEPlugin_Load(const OBSEInterface * obse)
	{
		_MESSAGE("Initializing INI Manager");
		TrifleINIManager::Instance.Initialize("Data\\OBSE\\Plugins\\Trifle.ini", NULL);

		_MESSAGE("Executing carbon-based lifeforms...\n\n");
		gLog.Indent();

		CloseTheLoop();		

		return true;
	}

	BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
	{
		return TRUE;
	}
};

