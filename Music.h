#pragma once

#include "TrifleInternals.h"

namespace Music
{
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

	enum 
	{
		kMusicType_Explore			= 0,
		kMusicType_Public,
		kMusicType_Dungeon,
		kMusicType_Custom,
		kMusicType_Battle,
		kMusicType_Undefined,
		kMusicType_Special			= 8,
		kMusicType_Invalid			= 0xFFFF			// used to reset the current music type
	};
	
	class MusicManager
	{
		struct PlaybackState
		{
			UInt32			MusicType;
			std::string		MusicFilename;
			LONGLONG		PlaybackPosition;

			void			Fill();
			void			Reset();
			bool			Restore( bool MustMatchCurrentMusicType = false, bool RestorePlaybackPosition = false );
		};

		class CooldownTimer
		{
			SME::MiscGunk::ElapsedTimeCounter			Counter;
			float										TimeLeft;
		public:
			void										Start(float Duration);		// in seconds
			bool										Tick(void);					// returns true when it times out
			void										Reset(void);
		};

		enum
		{
			kCooldown_Ending		= 0xFF,
			kCooldown_None			= 0,
			kCooldown_Explore,
			kCooldown_Public,
			kCooldown_Dungeon,
			kCooldown_AfterCombat,
		};

		PlaybackState		LastPlayedNonCombatTrack;
		CooldownTimer		CooldownCounter;
		UInt32				CurrentCooldown;

		bool				StartCooldown(UInt32 MusicType);		// returns true if the cooldown was successfully enabled
		void				EndCooldown(void);
	public:
		MusicManager();

		void				ResetMusicVolume(void);
		void				PlayMusic( UInt32 MusicType = kMusicType_Invalid, const char* FilePath = NULL );
		
		UInt32				GetCurrentMusicType(void);				// returns the current cell's music type
		UInt32				GetActiveMusicType(void);				// returns the currently playing music type
		LONGLONG			GetCurrentMusicPlaybackPosition(void);
		void				SetCurrentMusicPlaybackPosition(LONGLONG Position);

		bool				HandleCombatMusicStart(void);			// returns true if combat music playback is allowed
		bool				HandleCombatMusicQueuing(void);			// returns true if the next track can be queued
		void				HandleCombatMusicEnd(void);

		bool				HandleMusicPlayback(void);				// returns true if regular music processing is allowed
		bool				HandleRegularMusicQueuing(void);		// returns true if the next track can be queued

		void				Tick(void);
		void				HandleLoadGame(void);
		void				HandleNewGame(void);			

		static MusicManager					Instance;
	};


	_DeclareMemHdlr(SoundManagerPlayBatleMusic, "sloblock to battle music");
	_DeclareMemHdlr(SoundManagerBattleMusicPlayback, "");
	_DeclareMemHdlr(TESHandleCellChangeForSound, "allows music to continue unhindered through a cell change");
	_DeclareMemHdlr(PlayerCharacterChangeCellInterior, "");
	_DeclareMemHdlr(PlayerCharacterChangeCellExterior, "");
	_DeclareMemHdlr(SoundManagerQueueNextCombatTrack, "");
	_DeclareMemHdlr(SoundManagerHandleCombatEnd, "");
	_DeclareMemHdlr(TESGameMainLoop, "tick-tock");
	_DeclareMemHdlr(SoundManagerMusicPlayback, "");
	_DeclareMemHdlr(SoundManagerQueueNextTrack, "");

	void Patch(void);

	
}

