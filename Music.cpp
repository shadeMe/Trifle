#include "Music.h"
#include <dsound.h>
#include <dshow.h>

namespace Music
{
	Music::MusicManager MusicManager::Instance;


	void MusicManager::PlaybackState::Fill()
	{
		OSSoundGlobals* SoundManager = (*g_osGlobals)->sound;

		MusicType = SoundManager->musicType;
		MusicFilename = SoundManager->musicFileName;
		PlaybackPosition = MusicManager::Instance.GetCurrentMusicPlaybackPosition();
	}

	void MusicManager::PlaybackState::Reset()
	{
		MusicType = kMusicType_Invalid;
		MusicFilename.clear();
		PlaybackPosition = -1;
	}

	bool MusicManager::PlaybackState::Restore( bool MustMatchCurrentMusicType /*= false*/, bool RestorePlaybackPosition /*= false */ )
	{
		OSSoundGlobals* SoundManager = (*g_osGlobals)->sound;
		bool Result = false;

		if (MusicType != kMusicType_Invalid && MusicFilename.empty() == false)
		{
			if (MustMatchCurrentMusicType == false || MusicType == MusicManager::Instance.GetCurrentMusicType())
			{
#ifndef NDEBUG
				_MESSAGE("Restoring music state: File = %s, Type = %d, Position = %d, ", MusicFilename.c_str(), MusicType, PlaybackPosition);
#endif // !NDEBUG
				Result = true;
				MusicManager::Instance.PlayMusic(MusicType, MusicFilename.c_str());

				if (PlaybackPosition != -1 && RestorePlaybackPosition)
					MusicManager::Instance.SetCurrentMusicPlaybackPosition(PlaybackPosition);
			}
		}

		return Result;
	}

	void MusicManager::CooldownTimer::Start( float Duration )
	{
#ifndef NDEBUG
		_MESSAGE("CooldownTimer set to %f seconds", Duration);
#endif // !NDEBUG

		TimeLeft = Duration;
		Counter.Update();
	}

	bool MusicManager::CooldownTimer::Tick( void )
	{
		Counter.Update();

		double TimePassed = Counter.GetTimePassed();
#ifndef NDEBUG
	//	_MESSAGE("CooldownTimer elapsed %f ms", TimePassed);
#endif // !NDEBUG

		TimeLeft -= TimePassed / 1000.0f ;

		if (TimeLeft <= 0)
			return true;
		else
			return false;
	}

	void MusicManager::CooldownTimer::Reset( void )
	{
		TimeLeft = 0;
	}

	MusicManager::MusicManager() :
		LastPlayedNonCombatTrack(),
		CooldownCounter(),
		CurrentCooldown(kCooldown_None)
	{
		;//
	}

	void MusicManager::ResetMusicVolume( void )
	{
		thisCall<UInt32>(0x006AA1A0, (*g_osGlobals)->sound, (*g_osGlobals)->sound->musicVolume, true);
	}

	void MusicManager::PlayMusic( UInt32 MusicType /*= kMusicType_Invalid*/, const char* FilePath /*= NULL */ )
	{
		if (MusicType == kMusicType_Invalid)
			MusicType = GetCurrentMusicType();

		if (thisCall<bool>(0x006AB160, (*g_osGlobals)->sound, MusicType, FilePath, false))
		{
			thisCall<UInt32>(0x006AB420, (*g_osGlobals)->sound);
		}
		else
		{
#ifndef NDEBUG
			_MESSAGE("Couldn't queue file for playback - File = %s, Type = %d", FilePath, MusicType);
#endif // !NDEBUG
		}
	}

	UInt32 MusicManager::GetCurrentMusicType( void )
	{
		TESObjectCELL* Cell = thisCall<TESObjectCELL*>(0x00440560, TES::GetSingleton());
		UInt32 MusicType = 0;

		if (Cell)
		{
			MusicType = thisCall<UInt32>(0x004CAD00, Cell, 0);
		}

		return MusicType;
	}

	UInt32 MusicManager::GetActiveMusicType( void )
	{
		return (*g_osGlobals)->sound->musicType;
	}

	LONGLONG MusicManager::GetCurrentMusicPlaybackPosition( void )
	{
		OSSoundGlobals* SoundManager = (*g_osGlobals)->sound;
		IMediaSeeking* SeekInterface = NULL;
		LONGLONG Position = -1, Duration = -1;

		SoundManager->filterGraph->QueryInterface(IID_IMediaSeeking, (void**)&SeekInterface);
		if (SeekInterface)
		{
			SeekInterface->GetDuration(&Duration);
			SeekInterface->GetCurrentPosition(&Position);
		}
		else
		{
#ifndef NDEBUG
			_MESSAGE("Couldn't initialize filter graph interfaces!");
#endif // !NDEBUG
		}

		SAFERELEASE_D3D(SeekInterface);
		return Position;
	}

	void MusicManager::SetCurrentMusicPlaybackPosition( LONGLONG Position )
	{
		OSSoundGlobals* SoundManager = (*g_osGlobals)->sound;
		IMediaSeeking* SeekInterface = NULL;

		SoundManager->filterGraph->QueryInterface(IID_IMediaSeeking, (void**)&SeekInterface);

		if (SeekInterface)
		{
			LONGLONG Duration = 0;
			SeekInterface->GetDuration(&Duration);

			// we are using absolute positions
			if (Position < Duration)
			{
				HRESULT Result = S_OK;
				if ((Result = SeekInterface->SetPositions(&Position, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning)) != S_OK)
				{
#ifndef NDEBUG
					_MESSAGE("Couldn't set playback of the current track '%s' (Error: %d); Position = %d, Duration = %d",
						SoundManager->musicFileName, Result, Position, Duration);
#endif // !NDEBUG
				}
			}
		}
		else
		{
#ifndef NDEBUG
			_MESSAGE("Couldn't initialize filter graph interfaces!");
#endif // !NDEBUG
		}

		SAFERELEASE_D3D(SeekInterface);
	}

	bool MusicManager::HandleCombatMusicStart( void )
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

		// save current state
		if (AllowMusicStart && GetCurrentMusicType() != kMusicType_Battle)
		{
			LastPlayedNonCombatTrack.Fill();
		}

		return AllowMusicStart;
	}

	bool MusicManager::HandleCombatMusicQueuing( void )
	{
		bool Result = true;

		if (PlayerCombatState::Current() == false && GetActiveMusicType() == kMusicType_Battle)
		{
			// combat's done, back to our original program
			Result = false;

			// turn on the after-combat cooldown if it's enabled
			if (StartCooldown(kMusicType_Battle))
				return Result;

			if (Settings::kBattleMusicStopImmediatelyOnCombatEnd.GetData().i == 0)
			{
				if (Settings::kBattleMusicStartPreviousTrackOnCombatEnd.GetData().i)
				{
					if (LastPlayedNonCombatTrack.Restore(true, Settings::kBattleMusicRestorePreviousTrackPlaybackPosition.GetData().i))
						return Result;
					else
					{
#ifndef NDEBUG
						_MESSAGE("Couldn't restore previous track. Did the player switch cells?");
#endif // !NDEBUG
					}
				}

				// fallback to the usual, pick the current music type and play it
				PlayMusic();
			}
			else
			{
				// this should never happen
				_MESSAGE("More strangeness! Combat's done and the music is set to stop immediately and yet we are here...");
				PlayMusic();
			}
		}
		else
			;// do nothing, let the engine pick the next combat track

		return Result;
	}

	void MusicManager::HandleCombatMusicEnd( void )
	{
		if (Settings::kBattleMusicStopImmediatelyOnCombatEnd.GetData().i)
		{
			if (StartCooldown(kMusicType_Battle))
				return;

			if (Settings::kBattleMusicStartPreviousTrackOnCombatEnd.GetData().i)
			{
				if (LastPlayedNonCombatTrack.Restore(true, Settings::kBattleMusicRestorePreviousTrackPlaybackPosition.GetData().i))
					return;
				else
				{
#ifndef NDEBUG
					_MESSAGE("Couldn't restore previous track. Did the player switch cells?");
#endif // !NDEBUG
				}
			}

			// fallback to the usual, pick the current music type and play it
			PlayMusic();
		}
		else
			;// nothing to see here, HandleCombatMusicQueuing() will take care of the rest
	}

	void MusicManager::Tick( void )
	{
		if (CurrentCooldown != kCooldown_None && CurrentCooldown != kCooldown_Ending)
		{
			if (CooldownCounter.Tick())
			{
#ifndef NDEBUG
				_MESSAGE("Cooldown type %d ending...", CurrentCooldown);
#endif // !NDEBUG

				EndCooldown();
			}
		}
	}

	bool MusicManager::StartCooldown( UInt32 MusicType )
	{
		SME_ASSERT(CurrentCooldown == kCooldown_None);

		SME::INI::INISetting* Enabled = NULL;
		SME::INI::INISetting* Range = NULL;

		UInt32 CooldownType = kCooldown_None;
		switch (MusicType)
		{
		case kMusicType_Explore:
			CooldownType = kCooldown_Explore;
			break;
		case  kMusicType_Public:
			CooldownType = kCooldown_Public;
			break;
		case  kMusicType_Dungeon:
			CooldownType = kCooldown_Dungeon;
			break;
		case  kMusicType_Battle:
			CooldownType = kCooldown_AfterCombat;
			break;
		}

		SME_ASSERT(CooldownType != kCooldown_None);

		switch (CooldownType)
		{
		case kCooldown_AfterCombat:
			Enabled = &Settings::kCooldownAfterCombatEnabled;
			Range = &Settings::kCooldownAfterCombatRange;
			break;
		case kCooldown_Explore:
			Enabled = &Settings::kCooldownExploreEnabled;
			Range = &Settings::kCooldownExploreRange;
			break;
		case kCooldown_Public:
			Enabled = &Settings::kCooldownPublicEnabled;
			Range = &Settings::kCooldownPublicRange;
			break;
		case kCooldown_Dungeon:
			Enabled = &Settings::kCooldownDungeonEnabled;
			Range = &Settings::kCooldownDungeonRange;
			break;
		}

		SME_ASSERT(Enabled && Range);

		if (Enabled->GetData().i == 0)
			return false;
		
		std::string RangeString = Range->GetData().s;
		int Separator = 0;
		if (RangeString.length() < 3 || (Separator = RangeString.find("-")) == std::string::npos || RangeString.length() < Separator + 1)
		{
#ifndef NDEBUG
			_MESSAGE("Range string %s for cooldown type %d invalid!", RangeString.c_str(), CooldownType);
#endif // !NDEBUG

			return false;
		}

		std::string StartString = RangeString.substr(0, Separator);
		std::string EndString = RangeString.substr(Separator + 1);

		double MinTime = atof(StartString.c_str());
		double MaxTime = atof(EndString.c_str());

		if (StartString.empty() || EndString.empty() || errno == ERANGE)
		{
#ifndef NDEBUG
			_MESSAGE("Couldn't extract range from string %s for cooldown type %d!", RangeString.c_str(), CooldownType);
#endif // !NDEBUG

			return false;
		}

		if (MinTime > MaxTime)
		{
			double Buffer = MaxTime;
			MaxTime = MinTime;
			MinTime = Buffer;
		}

#ifndef NDEBUG
		_MESSAGE("Cooldown type %d has a range of %f-%f", CooldownType, MinTime, MaxTime);
#endif // !NDEBUG	

		SME::MersenneTwister::init_genrand(GetTickCount());
		double Duration = SME::MersenneTwister::genrand_real2() * (MaxTime - MinTime);
		Duration += MinTime;

#ifndef NDEBUG
		_MESSAGE("Cooldown type %d set to %f earth minutes", CooldownType, Duration);
#endif // !NDEBUG	

		CurrentCooldown = CooldownType;
		CooldownCounter.Start(Duration * 60.0f);

		return true;
	}

	void MusicManager::EndCooldown( void )
	{
		SME_ASSERT(CurrentCooldown != kCooldown_None && CurrentCooldown != kCooldown_Ending);

		CurrentCooldown = kCooldown_Ending;
		CooldownCounter.Reset();
	}

	void MusicManager::HandleNewGame( void )
	{
		ResetMusicVolume();
		PlayMusic();
	}

	void MusicManager::HandleLoadGame( void )
	{
		ResetMusicVolume();
		PlayMusic();
	}

	bool MusicManager::HandleMusicPlayback( void )
	{
		if (CurrentCooldown == kCooldown_None)
			return true;
		else if (CurrentCooldown == kCooldown_Ending)
		{
#ifndef NDEBUG
			_MESSAGE("Resuming regular processing...");
#endif // !NDEBUG

			CurrentCooldown = kCooldown_None;

			// make sure at least one track gets played before the next cooldown
			if (GetActiveMusicType() == kMusicType_Battle)
			{
				if (Settings::kBattleMusicStartPreviousTrackOnCombatEnd.GetData().i)
				{
					if (LastPlayedNonCombatTrack.Restore(true, Settings::kBattleMusicRestorePreviousTrackPlaybackPosition.GetData().i))
						return false;
				}
			}

			PlayMusic();
		}
		else
		{
			if (CurrentCooldown != kCooldown_AfterCombat)
			{
				// the rest of the cooldowns can be interrupted by combat
				if (Settings::kMusicAllowCombatToInterruptCooldown.GetData().i)
				{
					// manually check if the player's in combat
					PlayerCombatState::Evaluate();
					if (HandleCombatMusicStart())
					{
						// battle stations! resume regular processing
						EndCooldown();
						PlayMusic(kMusicType_Battle);
					}
				}
			}
		}

		return false;
	}

	bool MusicManager::HandleRegularMusicQueuing( void )
	{
		UInt32 MusicType = GetActiveMusicType();

		switch (MusicType)
		{
		case kMusicType_Battle:
			_MESSAGE("WTF! Combat music queuing happens elsewhere! Didn't you know that, you useless game!");
			return true;
		case kMusicType_Custom:
		case kMusicType_Special:
		case kMusicType_Undefined:
		case kMusicType_Invalid:
			_MESSAGE("I'll watch an entire episode of The Krypton Factor if this happens, I kid you not...");
			return true;
		default:
			return StartCooldown(MusicType) == false;
		}
	}


	
	_DefineHookHdlr(SoundManagerPlayBatleMusic, 0x00616285);
	_DefineHookHdlr(SoundManagerBattleMusicPlayback, 0x006AD9FC);
	_DefineJumpHdlr(TESHandleCellChangeForSound, 0x00440BBC, 0x00440BDA);
	_DefineJumpHdlr(PlayerCharacterChangeCellInterior, 0x0066EBE2, 0x0066EC17);
	_DefineJumpHdlr(PlayerCharacterChangeCellExterior, 0x0066ECA9, 0x0066ECD8);
	_DefineHookHdlr(SoundManagerQueueNextCombatTrack, 0x006AD787);
	_DefineHookHdlr(SoundManagerHandleCombatEnd, 0x006ADA2D);
	_DefineHookHdlr(TESGameMainLoop, 0x0040D817);
	_DefineHookHdlr(SoundManagerMusicPlayback, 0x006AD054);
	_DefineHookHdlr(SoundManagerQueueNextTrack, 0x006AD6DE);

	bool __stdcall EvaluateBattleMusicPlaybackConditions(void)
	{
		return MusicManager::Instance.HandleCombatMusicStart();
	}

	void __stdcall DoSoundManagerPlayBatleMusicHook(void)
	{
		PlayerCombatState::Evaluate();

		if (MusicManager::Instance.HandleCombatMusicStart())
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
		return MusicManager::Instance.HandleCombatMusicQueuing();
	}

	#define _hhName	SoundManagerQueueNextCombatTrack
	_hhBegin()
	{
		_hhSetVar(Retn, 0x006AD794);
		_hhSetVar(Skip, 0x006ADDF5);
		_hhSetVar(Call, 0x006A8E80);
		__asm
		{
			xor		eax, eax
			call	_hhGetVar(Call)
			push	eax
			call	DoSoundManagerQueueNextCombatTrackHook
			test	al, al
			jz		EXIT

			jmp		_hhGetVar(Retn)
		EXIT:
			jmp		_hhGetVar(Skip)
		}
	}

	void __stdcall DoSoundManagerHandleCombatEndHook(void)
	{
		MusicManager::Instance.HandleCombatMusicEnd();
	}

	#define _hhName	SoundManagerHandleCombatEnd
	_hhBegin()
	{
		_hhSetVar(Retn, 0x006ADDF5);
		__asm
		{			
			call	DoSoundManagerHandleCombatEndHook

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall DoTESGameMainLoopHook(void)
	{
		MusicManager::Instance.Tick();
	}

	#define _hhName	TESGameMainLoop
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040D81C);
		_hhSetVar(Call, 0x00578F60);
		__asm
		{
			pushad
			call	DoTESGameMainLoopHook
			popad

			call	_hhGetVar(Call)
			jmp		_hhGetVar(Retn)
		}
	}

	bool __stdcall DoSoundManagerMusicPlaybackHook(void)
	{
		return MusicManager::Instance.HandleMusicPlayback();
	}

	#define _hhName	SoundManagerMusicPlayback
	_hhBegin()
	{
		_hhSetVar(Retn, 0x006AD05A);
		_hhSetVar(Skip, 0x006ADDF8);
		__asm
		{
			call	DoSoundManagerMusicPlaybackHook
			test	al, al
			jz		SKIPPROCESSING

			mov		ecx, [esi + 0x70]
			test	ecx, ecx
			push	ebx
			jmp		_hhGetVar(Retn)
	SKIPPROCESSING:
			jmp		_hhGetVar(Skip)
		}
	}

	bool __stdcall DoSoundManagerQueueNextTrackHook(void)
	{
		return MusicManager::Instance.HandleRegularMusicQueuing();
	}

	#define _hhName	SoundManagerQueueNextTrack
	_hhBegin()
	{
		_hhSetVar(Retn, 0x006AD6E4);
		_hhSetVar(Skip, 0x006ADDF5);
		__asm
		{
			call	DoSoundManagerQueueNextTrackHook
			test	al, al
			jz		SKIPPROCESSING

			fld1
			push	ecx
			fstp	dword ptr [esp]
			jmp		_hhGetVar(Retn)
	SKIPPROCESSING:
			jmp		_hhGetVar(Skip)
		}
	}


	void Patch(void)
	{
		_MemHdlr(SoundManagerPlayBatleMusic).WriteJump();
		_MemHdlr(SoundManagerBattleMusicPlayback).WriteJump();
		_MemHdlr(SoundManagerQueueNextCombatTrack).WriteJump();
		_MemHdlr(SoundManagerHandleCombatEnd).WriteJump();
		_MemHdlr(TESGameMainLoop).WriteJump();
		_MemHdlr(SoundManagerMusicPlayback).WriteJump();
		_MemHdlr(SoundManagerQueueNextTrack).WriteJump();

		if (Settings::kMusicQueueImmediatelyOnCellChange.GetData().i == 0)
		{
			_MemHdlr(TESHandleCellChangeForSound).WriteJump();
			_MemHdlr(PlayerCharacterChangeCellInterior).WriteJump();
			_MemHdlr(PlayerCharacterChangeCellExterior).WriteJump();
		}
	}
}
