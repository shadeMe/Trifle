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


	_DeclareMemHdlr(SoundManagerPlayBatleMusic, "sloblock to battle music");
	_DeclareMemHdlr(SoundManagerBattleMusicPlayback, "");
	_DeclareMemHdlr(TESHandleCellChangeForSound, "allows music to continue unhindered through a cell change");
	_DeclareMemHdlr(PlayerCharacterChangeCellInterior, "");
	_DeclareMemHdlr(PlayerCharacterChangeCellExterior, "");
	_DeclareMemHdlr(SoundManagerQueueNextCombatTrack, "");
	_DeclareMemHdlr(SoundManagerHandleCombatEnd, "");

	void Patch(void);

	void LoadGameCallback(void);
	void NewGameCallback(void);
}

