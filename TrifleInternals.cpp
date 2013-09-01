#include "TrifleInternals.h"


namespace Interfaces
{
	PluginHandle					kOBSEPluginHandle = kPluginHandle_Invalid;
}

TrifleINIManager					TrifleINIManager::Instance;

namespace Settings
{
	SME::INI::INISetting			kDisableBattleMusic("DisableBattleMusic", "General", "What it says", (SInt32)0);
}

void TrifleINIManager::Initialize( const char* INIPath, void* Parameter )
{
	this->INIFilePath = INIPath;
	_MESSAGE("INI Path: %s", INIPath);

	std::fstream INIStream(INIPath, std::fstream::in);
	bool CreateINI = false;

	if (INIStream.fail())
	{
		_MESSAGE("INI File not found; Creating one...");
		CreateINI = true;
	}

	INIStream.close();
	INIStream.clear();

	RegisterSetting(&Settings::kDisableBattleMusic);

	if (CreateINI)
		Save();
}

_DefineNopHdlr(SoundManagerPlayBatleMusic, 0x00616285, 5);

void CloseTheLoop(void)
{
	if (Settings::kDisableBattleMusic.GetData().i)
	{
		_MemHdlr(SoundManagerPlayBatleMusic).WriteNop();
	}
}
