#include "TrifleInternals.h"


namespace Interfaces
{
	PluginHandle					kOBSEPluginHandle = kPluginHandle_Invalid;
}

TrifleINIManager					TrifleINIManager::Instance;

enum
{
	kBattleMusicPlaybackMode_Disabled		= 1 << 0,
	kBattleMusicPlaybackMode_Enabled		= 1 << 1,
	kBattleMusicPlaybackMode_DieRoll		= 1 << 2,
	kBattleMusicPlaybackMode_Distance		= 1 << 3,
	kBattleMusicPlaybackMode_Level			= 1 << 4,
};

namespace Settings
{
	SME::INI::INISetting			kBattleMusicPlaybackMode("PlaybackMode", "BattleMusic",
															"Battle music playback mode", (SInt32)1);

	SME::INI::INISetting			kBattleMusicMinimumEnemyDistance("MinimumEnemyDistance", "BattleMusic",
																	"Minimum distance b'ween the player and their nearest enemy", (float)1000.0f);

	SME::INI::INISetting			kBattleMusicMinimumEnemyLevel("EnemyLevelDelta", "BattleMusic",
																"Minimum level difference b'ween the player and their nearest enemy", (SInt32)0);

	SME::INI::INISetting			kBattleMusicDieRollChance("ChanceOfPlayback", "BattleMusic",
																"Chance, in percentage, of successful playback", (SInt32)50);
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

	RegisterSetting(&Settings::kBattleMusicPlaybackMode);
	RegisterSetting(&Settings::kBattleMusicMinimumEnemyDistance);
	RegisterSetting(&Settings::kBattleMusicMinimumEnemyLevel);
	RegisterSetting(&Settings::kBattleMusicDieRollChance);

	if (CreateINI)
		Save();
}

_DefineHookHdlr(SoundManagerPlayBatleMusic, 0x00616285);
_DefineHookHdlr(SoundManagerMusicPlayback, 0x006AD9FC);
_DefineHookHdlr(TESDataHandlerPopulatePluginList, 0x0044A3D4);

bool __stdcall EvaluateBattleMusicPlaybackConditions(void)
{
	static bool kLastCombatState = false;

	bool PlayerInCombat = thisCall<bool>(0x006605A0, *g_thePlayer, false);
	bool PlayerExitedCombat = false;
	bool PlayerEnteredCombat = false;
	bool CombatStateChanged = false;

	if (PlayerInCombat != kLastCombatState)
	{
		CombatStateChanged = true;

		if (kLastCombatState == true && PlayerInCombat == false)
			PlayerExitedCombat = true;
		else if (kLastCombatState == false && PlayerInCombat == true)
			PlayerEnteredCombat = true;

		kLastCombatState = PlayerInCombat;
	}

	UInt32 PlaybackMode = Settings::kBattleMusicPlaybackMode.GetData().i;

	if (PlayerInCombat)
	{
		PlayerInCombat =  thisCall<bool>(0x006605A0, *g_thePlayer, true);

		if (PlayerInCombat)
		{
			PlayerInCombat = false;
			if ((PlaybackMode & kBattleMusicPlaybackMode_Enabled))
				PlayerInCombat = true;
			else if ((PlaybackMode & kBattleMusicPlaybackMode_Disabled) == false)
			{

			}

			PlayerInCombat = true;
			_MESSAGE("Evaluating combat music...");
		}
	}

	return PlayerInCombat;
}

void __stdcall DoSoundManagerPlayBatleMusicHook(void)
{
	if (EvaluateBattleMusicPlaybackConditions())
		cdeclCall<void>(0x006136B0);
}

#define _hhName	SoundManagerPlayBatleMusic
_hhBegin()
{
	_hhSetVar(Retn, 0x0061628A);
	__asm
	{
		pushad
		call	DoSoundManagerPlayBatleMusicHook
		popad

		jmp		[_hhGetVar(Retn)]
	}
}

#define _hhName	SoundManagerMusicPlayback
_hhBegin()
{
	_hhSetVar(SkipMusic, 0x006ADA1F);
	_hhSetVar(AllowMusic, 0x006ADC15);
	_hhSetVar(PlayerInCombat, 0x006605A0);
	__asm
	{
		cmp		word ptr [esi + 0xB0], 0x4
		jz		CHECKCOMBATMUSIC

		mov		ecx, 0x00B333C4
		mov		ecx, [ecx]
		push	0
		call	[_hhGetVar(PlayerInCombat)]
		test	al, al
		jz		CHECKCOMBATMUSIC

	SKIPCOMBATMUSIC:
		jmp		[_hhGetVar(SkipMusic)]
	CHECKCOMBATMUSIC:
		call	EvaluateBattleMusicPlaybackConditions
		test	al, al
		jz		SKIPCOMBATMUSIC

		jmp		[_hhGetVar(AllowMusic)]
	}
}

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
	_MemHdlr(SoundManagerPlayBatleMusic).WriteJump();
	_MemHdlr(SoundManagerMusicPlayback).WriteJump();
	_MemHdlr(TESDataHandlerPopulatePluginList).WriteJump();

	SME::MersenneTwister::init_genrand(GetTickCount());
}
