#include "TrifleInternals.h"


namespace Interfaces
{
	PluginHandle					kOBSEPluginHandle = kPluginHandle_Invalid;

	OBSESerializationInterface*		kOBSESerialization = NULL;
	OBSEMessagingInterface*			kOBSEMessaging = NULL;
}

TrifleINIManager					TrifleINIManager::Instance;

enum
{
	kBattleMusicPlaybackMode_Disabled		= 0,
	kBattleMusicPlaybackMode_Enabled		= 1,
	kBattleMusicPlaybackMode_DieRoll		= 2,

	// the following can be combined bit-wise
	kBattleMusicPlaybackMode_Distance		= 1 << 2,
	kBattleMusicPlaybackMode_Level			= 1 << 3,
	kBattleMusicPlaybackMode_FirstBlood		= 1 << 4,
};

namespace Settings
{
	SME::INI::INISetting			kBattleMusicPlaybackMode("PlaybackMode", "BattleMusic",
															"Battle music playback mode", (SInt32)kBattleMusicPlaybackMode_Enabled);

	SME::INI::INISetting			kBattleMusicMaximumEnemyDistance("MaximumEnemyDistance", "BattleMusic",
																	"Maximum distance b'ween the player and their nearest enemy", (float)1000.0f);

	SME::INI::INISetting			kBattleMusicEnemyLevelDelta("EnemyLevelDelta", "BattleMusic",
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
	RegisterSetting(&Settings::kBattleMusicMaximumEnemyDistance);
	RegisterSetting(&Settings::kBattleMusicEnemyLevelDelta);
	RegisterSetting(&Settings::kBattleMusicDieRollChance);

	if (CreateINI)
		Save();
}

_DefineHookHdlr(SoundManagerPlayBatleMusic, 0x00616285);
_DefineHookHdlr(SoundManagerMusicPlayback, 0x006AD9FC);
_DefineHookHdlr(TESDataHandlerPopulatePluginList, 0x0044A3D4);
_DefinePatchHdlr(PlayerCharacterOnHealthDamage, 0x00A73DC4);

namespace PlayerCombatState
{
	static bool CombatFinish = false;
	static bool CombatStart = false;
	static bool StateChanged = false;

	static Actor* LastKnownAttacker = NULL;

	bool Current(void)
	{
		return thisCall<bool>(0x006605A0, *g_thePlayer, false);
	}

	bool Evaluate(void)
	{
		static bool kLastCombatState = false;

		// reset state vars
		CombatFinish = false;
		CombatStart = false;
		StateChanged = false;

		bool CurrentState = thisCall<bool>(0x006605A0, *g_thePlayer, false);

		if (CurrentState != kLastCombatState)
		{
#ifndef NDEBUG
			_MESSAGE("Player combat state changed to: %s", (CurrentState ? "In Combat" : "Not In Combat"));
#endif // !NDEBUG

			StateChanged = true;

			if (kLastCombatState == true && CurrentState == false)
				CombatFinish = true;
			else if (kLastCombatState == false && CurrentState == true)
				CombatStart = true;

			kLastCombatState = CurrentState;
			LastKnownAttacker = NULL;
		}

		return CurrentState;
	}

	void GetNearestCombatant(Actor*& OutNearest, long double& OutDistance)
	{
		tList<Actor>* CombatActors = *((tList<Actor>**)((UInt32)(*g_thePlayer) + 0x5AC));
		Actor* Nearest = NULL;
		long double Distance = 0;

		if (CombatActors)
		{
			for (tList<Actor>::Iterator Itr = CombatActors->Begin(); Itr.End() == false && Itr.Get(); ++Itr)
			{
				Actor* Current = Itr.Get();
				long double FromPlayer = thisCall<long double>(0x004D7E90, *g_thePlayer, Current, 1);

				if (Nearest == NULL || FromPlayer < Distance)
				{
					Nearest = Current;
					Distance = FromPlayer;
				}
			}
		}

		OutNearest = Nearest;
		OutDistance = Distance;
	}

	bool GetIsPlayerCombatant(Actor* Target)
	{
		tList<Actor>* CombatActors = *((tList<Actor>**)((UInt32)(*g_thePlayer) + 0x5AC));

		if (CombatActors)
		{
			for (tList<Actor>::Iterator Itr = CombatActors->Begin(); Itr.End() == false && Itr.Get(); ++Itr)
			{
				Actor* Current = Itr.Get();
				if (Current == Target)
					return true;
			}
		}

		return false;
	}
}

bool __stdcall EvaluateBattleMusicPlaybackConditions(void)
{
	bool InCombat = PlayerCombatState::Current();
	bool AllowMusicStart = false;
	UInt32 PlaybackMode = Settings::kBattleMusicPlaybackMode.GetData().i;

	if (InCombat)
	{
		if (PlaybackMode == kBattleMusicPlaybackMode_Enabled)
			AllowMusicStart = true;
		else if (PlaybackMode != kBattleMusicPlaybackMode_Disabled)
		{
#ifndef NDEBUG
	//		_MESSAGE("Evaluating battle music conditions...");
			gLog.Indent();
#endif // !NDEBUG

			if (PlaybackMode == kBattleMusicPlaybackMode_DieRoll)
			{
				// only checked when the player enters combat
				if (PlayerCombatState::CombatStart)
				{
					SInt32 SuccessChance = Settings::kBattleMusicDieRollChance.GetData().i;
					if (SuccessChance > 100)
						SuccessChance = 100;

					int Roll = cdeclCall<int>(0x0047DF80, 0) % 100 + 1;
#ifndef NDEBUG
					_MESSAGE("Rolled a %d...", Roll);
#endif // !NDEBUG
					if (Roll < SuccessChance || SuccessChance == 100)
					{
#ifndef NDEBUG
						_MESSAGE("Success!");
#endif // !NDEBUG
						AllowMusicStart = true;
					}
					else
					{
#ifndef NDEBUG
						_MESSAGE("Failed! Waiting until state change...");
#endif // !NDEBUG
					}
				}
			}
			else
			{
				// these are checked every frame
				Actor* Nearest = NULL;
				long double Distance = 0.f;
				PlayerCombatState::GetNearestCombatant(Nearest, Distance);

				if (Nearest == NULL)
				{
					_MESSAGE("Good news, everyone! The player's in combat but there is NO combatant in the vicinity! Let there be music...");

					AllowMusicStart = true;
				}
				else
				{
#ifndef NDEBUG
					if (Nearest)
						_MESSAGE("Nearest combatant %s (%08X) at %f units from the player...", Nearest->GetFullName()->name.m_data, Nearest->refID, Distance);
#endif // !NDEBUG

					if ((PlaybackMode & kBattleMusicPlaybackMode_Distance))
					{
						float Maximum = Settings::kBattleMusicMaximumEnemyDistance.GetData().f;
#ifndef NDEBUG
						_MESSAGE("Checking nearest combatant distance...");
#endif // !NDEBUG
						if (Distance < Maximum)
						{
#ifndef NDEBUG
							_MESSAGE("Success!");
#endif // !NDEBUG
							AllowMusicStart = true;
						}
					}

					if ((PlaybackMode & kBattleMusicPlaybackMode_Level))
					{
						SInt32 LevelDelta = Settings::kBattleMusicEnemyLevelDelta.GetData().i;
						UInt32 PlayerLevel = thisCall<UInt32>(0x005E1FD0, *g_thePlayer);
						UInt32 CombatantLevel = thisCall<UInt32>(0x005E1FD0, Nearest);
#ifndef NDEBUG
						_MESSAGE("Nearest combatant at level %d...", CombatantLevel);
#endif // !NDEBUG
						if (CombatantLevel >= (PlayerLevel + LevelDelta))
						{
#ifndef NDEBUG
							_MESSAGE("Success!");
#endif // !NDEBUG
							AllowMusicStart = true;
						}
					}
				}

				if ((PlaybackMode & kBattleMusicPlaybackMode_FirstBlood))
				{					
					if (PlayerCombatState::LastKnownAttacker && PlayerCombatState::GetIsPlayerCombatant(PlayerCombatState::LastKnownAttacker))
					{
#ifndef NDEBUG
						_MESSAGE("Actor %s damaged the player!", PlayerCombatState::LastKnownAttacker->GetFullName()->name.m_data);
#endif // !NDEBUG
						AllowMusicStart = true;
					}
				}
			}
#ifndef NDEBUG
			gLog.Outdent();
#endif // !NDEBUG
		}
	}

	return AllowMusicStart;
}

void __stdcall DoSoundManagerPlayBatleMusicHook(void)
{
	PlayerCombatState::Evaluate();

	if (EvaluateBattleMusicPlaybackConditions())
		cdeclCall<void>(0x006136B0);
}

#define _hhName	SoundManagerPlayBatleMusic
_hhBegin()
{
	_hhSetVar(Retn, 0x0061628A);
	__asm
	{
		call	DoSoundManagerPlayBatleMusicHook

		jmp		[_hhGetVar(Retn)]
	}
}

#define _hhName	SoundManagerMusicPlayback
_hhBegin()
{
	_hhSetVar(SkipMusic, 0x006ADA1F);
	_hhSetVar(AllowMusic, 0x006ADC15);
	_hhSetVar(AllowMusicAux, 0x006ADC44);
	_hhSetVar(PlayerInCombat, 0x006605A0);
	_hhSetVar(Exit, 0x006ADDF5);
	__asm
	{
		mov		ecx, 0x00B3C0EC
		movzx	ecx, byte ptr [ecx]	
		test	ecx, ecx
		jnz		CONTINUECURRENTMUSIC			// we're in the main menu, we better sod off

		call	PlayerCombatState::Evaluate
		mov		ah, 0
		cmp		word ptr [esi + 0xB0], 0x4
		jnz		CHECKCOMBATMUSIC				// combat music not playing

		mov		ecx, 0x00B333C4
		mov		ecx, [ecx]
		push	0
		call	[_hhGetVar(PlayerInCombat)]
		test	al, al
		jnz		RECHECKSNDMGR

	SKIPCOMBATMUSICEPILOG:
		jmp		[_hhGetVar(SkipMusic)]
	CHECKCOMBATMUSIC:
		call	EvaluateBattleMusicPlaybackConditions
		test	ah, ah							// check if we got here from the auxiliary location
		jnz		SKIPCOMBATMUSICPROLOG			// different exit point when combat music is already playing
		
		test	al, al
		jz		SKIPCOMBATMUSICEPILOG			
	
		jmp		[_hhGetVar(AllowMusic)]
	RECHECKSNDMGR:
		movzx	eax, [esi + 0xB0]
		cmp		ax, 0x4
		jz		CONTINUECURRENTMUSIC
		cmp		ax, bx
		jz		CONTINUECURRENTMUSIC

		mov		ecx, 0x00B333C4
		mov		ecx, [ecx]
		push	1
		call	[_hhGetVar(PlayerInCombat)]
		test	al, al
		jz		CONTINUECURRENTMUSIC

		mov		ah, 1							// set high word to differentiate b'ween the entry points
		jmp		CHECKCOMBATMUSIC
	CONTINUECURRENTMUSIC:
		jmp		[_hhGetVar(Exit)]
	SKIPCOMBATMUSICPROLOG:
		test	al, al
		jz		CONTINUECURRENTMUSIC

		jmp		[_hhGetVar(AllowMusicAux)]
	}
}

UInt32 __stdcall DoPlayerCharacterOnHealthDamageHook(PlayerCharacter* PC, Actor* Attacker, float Damage)
{
	PlayerCombatState::LastKnownAttacker = Attacker;

#ifndef NDEBUG
	if (Attacker)
		_MESSAGE("Actor %s dealt the player %f points of health damage", Attacker->GetFullName()->name.m_data, Damage);
#endif // !NDEBUG

	return thisCall<UInt32>(0x0065D6F0, PC, Attacker, Damage);
}

#define _hhName	PlayerCharacterOnHealthDamage
_hhBegin()
{
	__asm
	{	
		mov		eax, [esp + 0x4]
		mov		edx, [esp + 0x8]
		push	edx
		push	eax
		push	ecx
		call	DoPlayerCharacterOnHealthDamageHook
		retn	0x8
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
	_MemHdlr(PlayerCharacterOnHealthDamage).WriteUInt32((UInt32)&PlayerCharacterOnHealthDamageHook);
}
