#pragma once

#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"
#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/GameData.h"
#include "obse/GameMenus.h"
#include "obse/GameOSDepend.h"
#include "obse/NiAPI.h"
#include "obse/NiObjects.h"
#include "obse/NiTypes.h"
#include "obse/ParamInfos.h"
#include "obse/GameActorValues.h"

#include <SME_Prefix.h>
#include <MemoryHandler.h>
#include <INIManager.h>
#include <StringHelpers.h>
#include <MersenneTwister.h>
#include <MiscGunk.h>


using namespace SME;
using namespace SME::MemoryHandler;

namespace Interfaces
{
	extern PluginHandle						kOBSEPluginHandle;

	extern OBSESerializationInterface*		kOBSESerialization;
	extern OBSEMessagingInterface*			kOBSEMessaging;
}

class TrifleINIManager : public INI::INIManager
{
public:
	void									Initialize(const char* INIPath, void* Parameter);

	static TrifleINIManager					Instance;
};

namespace Settings
{
	extern SME::INI::INISetting				kBattleMusicPlaybackMode;
	extern SME::INI::INISetting				kBattleMusicMaximumEnemyDistance;
	extern SME::INI::INISetting				kBattleMusicEnemyLevelDelta;
	extern SME::INI::INISetting				kBattleMusicDieRollChance;
	extern SME::INI::INISetting				kBattleMusicStopImmediatelyOnCombatEnd;
	extern SME::INI::INISetting				kBattleMusicStartPreviousTrackOnCombatEnd;
	extern SME::INI::INISetting				kBattleMusicRestorePreviousTrackPlaybackPosition;

	extern SME::INI::INISetting				kCooldownAfterCombatEnabled;
	extern SME::INI::INISetting				kCooldownAfterCombatRange;

	extern SME::INI::INISetting				kCooldownExploreEnabled;
	extern SME::INI::INISetting				kCooldownExploreRange;

	extern SME::INI::INISetting				kCooldownPublicEnabled;
	extern SME::INI::INISetting				kCooldownPublicRange;

	extern SME::INI::INISetting				kCooldownDungeonEnabled;
	extern SME::INI::INISetting				kCooldownDungeonRange;

	extern SME::INI::INISetting				kMusicQueueImmediatelyOnCellChange;
	extern SME::INI::INISetting				kMusicAllowCombatToInterruptCooldown;

	extern SME::INI::INISetting				kGraphicsEnablePlayerFirstPersonShadow;

	extern SME::INI::INISetting				kBugFixHorseCorpseCollision;


	extern SME::INI::INISetting				kPluginHooksMusic;
}

namespace PlayerCombatState
{
	extern bool CombatFinish;
	extern bool CombatStart;
	extern bool StateChanged;

	extern Actor* LastKnownAttacker;
	extern Actor* LastKnownAttackee;

	bool Current(void);
	bool Evaluate(void);
	void GetNearestCombatant(Actor*& OutNearest, long double& OutDistance);
	bool GetIsPlayerCombatant(Actor* Target);
}