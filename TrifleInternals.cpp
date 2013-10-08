#include "TrifleInternals.h"
#include "Music.h"

#pragma comment(lib, "Strmiids.lib")

namespace Interfaces
{
	PluginHandle					kOBSEPluginHandle = kPluginHandle_Invalid;

	OBSESerializationInterface*		kOBSESerialization = NULL;
	OBSEMessagingInterface*			kOBSEMessaging = NULL;
}

TrifleINIManager					TrifleINIManager::Instance;


namespace Settings
{
	SME::INI::INISetting	kBattleMusicPlaybackMode("PlaybackMode", "Music::Combat Playback",
													"Battle music playback mode", (SInt32)Music::kBattleMusicPlaybackMode_Enabled);

	SME::INI::INISetting	kBattleMusicMaximumEnemyDistance("MaximumEnemyDistance", "Music::Combat Playback",
															"Maximum distance b'ween the player and their nearest enemy", (float)1000.0f);

	SME::INI::INISetting	kBattleMusicEnemyLevelDelta("EnemyLevelDelta", "Music::Combat Playback",
														"Minimum level difference b'ween the player and their nearest enemy", (SInt32)0);

	SME::INI::INISetting	kBattleMusicDieRollChance("ChanceOfPlayback", "Music::Combat Playback",
													"Chance, in percentage, of successful playback", (SInt32)50);

	SME::INI::INISetting	kBattleMusicStopImmediatelyOnCombatEnd("StopImmediatelyOnCombatEnd", "Music::Combat Playback",
																"", (SInt32)1);

	SME::INI::INISetting	kBattleMusicStartPreviousTrackOnCombatEnd("StartPreviousTrackOnCombatEnd", "Music::Combat Playback",
																	"", (SInt32)1);

	SME::INI::INISetting	kBattleMusicRestorePreviousTrackPlaybackPosition("RestorePreviousTrackPlaybackPosition", "Music::Combat Playback",
																			"", (SInt32)0);


	SME::INI::INISetting	kCooldownAfterCombatEnabled("Enabled", "Music::Cooldown AfterCombat", "", (SInt32)0);
	SME::INI::INISetting	kCooldownAfterCombatRange("Range", "Music::Cooldown AfterCombat", "Range of the cooldown period, in earth minutes", "0.5-1.75");

	SME::INI::INISetting	kCooldownExploreEnabled("Enabled", "Music::Cooldown Explore", "", (SInt32)0);
	SME::INI::INISetting	kCooldownExploreRange("Range", "Music::Cooldown Explore", "Range of the cooldown period, in earth minutes", "0.5-1.75");

	SME::INI::INISetting	kCooldownPublicEnabled("Enabled", "Music::Cooldown Public", "", (SInt32)0);
	SME::INI::INISetting	kCooldownPublicRange("Range", "Music::Cooldown Public", "Range of the cooldown period, in earth minutes", "0.5-1.75");

	SME::INI::INISetting	kCooldownDungeonEnabled("Enabled", "Music::Cooldown Dungeon", "", (SInt32)0);
	SME::INI::INISetting	kCooldownDungeonRange("Range", "Music::Cooldown Dungeon", "Range of the cooldown period, in earth minutes", "0.5-1.75");


	SME::INI::INISetting	kMusicQueueImmediatelyOnCellChange("QueueImmediatelyOnCellChange", "Music::General",
															"Immediately change the current playing track to match the destination cell's music type", (SInt32)1);
	
	SME::INI::INISetting	kMusicAllowCombatToInterruptCooldown("AllowCombatToInterruptCooldown", "Music::General", "", (SInt32)1);


	SME::INI::INISetting	kGraphicsEnablePlayerFirstPersonShadow("EnableFirstPersonShadow", "Graphics::Player", "Show first person shadows for the PC", (SInt32)1);
#ifndef NDEBUG
	SME::INI::INISetting	kGraphicsEnablePlayerFirstPersonBody("EnableFirstPersonBody", "Graphics::Player", "Show first person body for the PC", (SInt32)1);
#else
	SME::INI::INISetting	kGraphicsEnablePlayerFirstPersonBody("EnableFirstPersonBody", "Graphics::Player", "Show first person body for the PC", (SInt32)0);
#endif // !NDEBUG



	SME::INI::INISetting	kBugFixHorseCorpseCollision("HorseCorpseCollision", "BugFix::General", "Fixes the jittering when riding a horse over a corpse", (SInt32)1);



	SME::INI::INISetting	kPluginHooksMusic("PatchMusic", "Plugin::Hooks", "", (SInt32)1);
}

void TrifleINIManager::Initialize( const char* INIPath, void* Parameter )
{
	this->INIFilePath = INIPath;
	_MESSAGE("INI Path: %s", INIPath);

	RegisterSetting(&Settings::kBattleMusicPlaybackMode);
	RegisterSetting(&Settings::kBattleMusicMaximumEnemyDistance);
	RegisterSetting(&Settings::kBattleMusicEnemyLevelDelta);
	RegisterSetting(&Settings::kBattleMusicDieRollChance);
	RegisterSetting(&Settings::kBattleMusicStopImmediatelyOnCombatEnd);
	RegisterSetting(&Settings::kBattleMusicStartPreviousTrackOnCombatEnd);
	RegisterSetting(&Settings::kBattleMusicRestorePreviousTrackPlaybackPosition);

	RegisterSetting(&Settings::kCooldownAfterCombatEnabled);
	RegisterSetting(&Settings::kCooldownAfterCombatRange);

	RegisterSetting(&Settings::kCooldownExploreEnabled);
	RegisterSetting(&Settings::kCooldownExploreRange);

	RegisterSetting(&Settings::kCooldownPublicEnabled);
	RegisterSetting(&Settings::kCooldownPublicRange);

	RegisterSetting(&Settings::kCooldownDungeonEnabled);
	RegisterSetting(&Settings::kCooldownDungeonRange);

	
	RegisterSetting(&Settings::kMusicQueueImmediatelyOnCellChange);
	RegisterSetting(&Settings::kMusicAllowCombatToInterruptCooldown);


	RegisterSetting(&Settings::kGraphicsEnablePlayerFirstPersonShadow);
#ifndef NDEBUG
	RegisterSetting(&Settings::kGraphicsEnablePlayerFirstPersonBody);
#endif // !NDEBUG

	RegisterSetting(&Settings::kBugFixHorseCorpseCollision);


	RegisterSetting(&Settings::kPluginHooksMusic);

	Save();
}

namespace PlayerCombatState
{
	bool CombatFinish = false;
	bool CombatStart = false;
	bool StateChanged = false;

	Actor* LastKnownAttacker = NULL;
	Actor* LastKnownAttackee = NULL;

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