#include "Music.h"

namespace Music
{
	void ResetMusicVolume( void )
	{
		thisCall<UInt32>(0x006AA1A0, (*g_osGlobals)->sound, (*g_osGlobals)->sound->musicVolume, true);
	}

	UInt32 GetCurrentMusicType( void )
	{
		TESObjectCELL* Cell = thisCall<TESObjectCELL*>(0x00440560, TES::GetSingleton());
		UInt32 MusicType = 0;

		if (Cell)
		{
			MusicType = thisCall<UInt32>(0x004CAD00, Cell, 0);
		}

		return MusicType;
	}

	void PlayCurrentMusic( void )
	{
		UInt32 MusicType = GetCurrentMusicType();

		if (thisCall<bool>(0x006AB160, (*g_osGlobals)->sound, MusicType, NULL, false))
			thisCall<UInt32>(0x006AB420, (*g_osGlobals)->sound);
	}
	
	_DefineHookHdlr(SoundManagerPlayBatleMusic, 0x00616285);
	_DefineHookHdlr(SoundManagerBattleMusicPlayback, 0x006AD9FC);
	_DefineJumpHdlr(TESHandleCellChangeForSound, 0x00440BBC, 0x00440BDA);
	_DefineJumpHdlr(PlayerCharacterChangeCellInterior, 0x0066EBE2, 0x0066EC17);
	_DefineJumpHdlr(PlayerCharacterChangeCellExterior, 0x0066ECA9, 0x0066ECD8);
	_DefineHookHdlr(SoundManagerQueueNextCombatTrack, 0x006AD787);
	_DefineJumpHdlr(SoundManagerHandleCombatEnd, 0x006ADA2D, 0x006ADDF5);

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

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	SoundManagerBattleMusicPlayback
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
			call	_hhGetVar(PlayerInCombat)
			test	al, al
			jnz		RECHECKSNDMGR

		SKIPCOMBATMUSICEPILOG:
			jmp		_hhGetVar(SkipMusic)
		CHECKCOMBATMUSIC:
			call	EvaluateBattleMusicPlaybackConditions
			test	ah, ah							// check if we got here from the auxiliary location
			jnz		SKIPCOMBATMUSICPROLOG			// different exit point when combat music is already playing
		
			test	al, al
			jz		SKIPCOMBATMUSICEPILOG			
	
			jmp		_hhGetVar(AllowMusic)
		RECHECKSNDMGR:
			movzx	eax, [esi + 0xB0]
			cmp		ax, 0x4
			jz		CONTINUECURRENTMUSIC
			cmp		ax, bx
			jz		CONTINUECURRENTMUSIC

			mov		ecx, 0x00B333C4
			mov		ecx, [ecx]
			push	1
			call	_hhGetVar(PlayerInCombat)
			test	al, al
			jz		CONTINUECURRENTMUSIC

			mov		ah, 1							// set high word to differentiate b'ween the entry points
			jmp		CHECKCOMBATMUSIC
		CONTINUECURRENTMUSIC:
			jmp		_hhGetVar(Exit)
		SKIPCOMBATMUSICPROLOG:
			test	al, al
			jz		CONTINUECURRENTMUSIC

			jmp		_hhGetVar(AllowMusicAux)
		}
	}

	bool __stdcall DoSoundManagerQueueNextCombatTrackHook(bool Result)
	{
		if (Settings::kBattleMusicStopImmediatelyOnCombatEnd.GetData().i == 0 &&
			PlayerCombatState::Current() == false &&
			(*g_osGlobals)->sound->musicType == kMusicType_Battle)
		{
			PlayCurrentMusic();
			Result = false;
		}

		return Result;
	}

	#define _hhName	SoundManagerQueueNextCombatTrack
	_hhBegin()
	{
		_hhSetVar(Retn, 0x006AD78C);
		_hhSetVar(Call, 0x006A8E80);
		__asm
		{
			call	_hhGetVar(Call)
			push	al
			call	DoSoundManagerQueueNextCombatTrackHook

			jmp		_hhGetVar(Retn)
		}
	}

	void Patch(void)
	{
		_MemHdlr(SoundManagerPlayBatleMusic).WriteJump();
		_MemHdlr(SoundManagerBattleMusicPlayback).WriteJump();
		_MemHdlr(SoundManagerQueueNextCombatTrack).WriteJump();

		if (Settings::kMusicQueueImmediatelyOnCellChange.GetData().i == 0)
		{
			_MemHdlr(TESHandleCellChangeForSound).WriteJump();
			_MemHdlr(PlayerCharacterChangeCellInterior).WriteJump();
			_MemHdlr(PlayerCharacterChangeCellExterior).WriteJump();
		}

		if (Settings::kBattleMusicStopImmediatelyOnCombatEnd.GetData().i == 0)
		{
			_MemHdlr(SoundManagerHandleCombatEnd).WriteJump();
		}
	}

	

	void LoadGameCallback( void )
	{
		ResetMusicVolume();
		PlayCurrentMusic();
	}

	void NewGameCallback( void )
	{
		ResetMusicVolume();
		PlayCurrentMusic();
	}
}
