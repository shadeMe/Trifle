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
_DefineHookHdlr(TESDataHandlerPopulatePluginList, 0x0044A3D4);

bool __stdcall FixPluginListPopulation(WIN32_FIND_DATA* FileData)
{
	static std::list<std::string> kActivePluginList;
	if (kActivePluginList.size() == 0)
	{
		const char* kAppDataPath = (const char*)0x00B3F178;
		const char* kPluginListName = ".\\Plugins.txt";

		char Buffer[0x104] = {0};
		strcpy_s(Buffer, sizeof(Buffer), kAppDataPath);
		strcat_s(Buffer, sizeof(Buffer), kPluginListName);

		std::fstream PluginListStream(Buffer, std::ios::in);
		if (PluginListStream.fail() == false)
		{
			char Entry[0x200] = {0};
			while (PluginListStream.eof() == false)
			{
				ZeroMemory(Entry, sizeof(Entry));
				PluginListStream.getline(Entry, sizeof(Entry));

				if (strlen(Entry) > 2 && Entry[0] != '#')
				{
					kActivePluginList.push_back(Entry);
				}
			}
		}
	}

	if (!_stricmp(FileData->cFileName, "Oblivion.esm"))
	{
		return true;
	}

	for (std::list<std::string>::const_iterator Itr = kActivePluginList.begin(); Itr != kActivePluginList.end(); Itr++)
	{
		if (!_stricmp(Itr->c_str(), FileData->cFileName))
			return true;
	}

	return false;
}

#define _hhName	TESDataHandlerPopulatePluginList
_hhBegin()
{
	_hhSetVar(Retn, 0x0044A3DA);
	_hhSetVar(Jump, 0x0044A3E4);
	_hhSetVar(Skip, 0x0044A514);
	__asm
	{
		lea     eax, [esp + 0x2C]

		pushad
		push	eax
		call	FixPluginListPopulation
		test	al, al
		jz		SKIPPLUGIN

		popad
		cmp     [esp + 0x48], ebx
		jnz		AWAY
		jmp		[_hhGetVar(Retn)]
	SKIPPLUGIN:
		popad
		jmp		[_hhGetVar(Skip)]
	AWAY:
		jmp		[_hhGetVar(Jump)]
	}
}

void CloseTheLoop(void)
{
	if (Settings::kDisableBattleMusic.GetData().i)
	{
		_MemHdlr(SoundManagerPlayBatleMusic).WriteNop();
	}

	_MemHdlr(TESDataHandlerPopulatePluginList).WriteJump();
}
