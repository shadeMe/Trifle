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
	SME::INI::INISetting			kBattleMusicPlaybackMode("PlaybackMode", "Music::CombatPlayback",
															"Battle music playback mode", (SInt32)Music::kBattleMusicPlaybackMode_Enabled);

	SME::INI::INISetting			kBattleMusicMaximumEnemyDistance("MaximumEnemyDistance", "Music::CombatPlayback",
																	"Maximum distance b'ween the player and their nearest enemy", (float)1000.0f);

	SME::INI::INISetting			kBattleMusicEnemyLevelDelta("EnemyLevelDelta", "Music::CombatPlayback",
																"Minimum level difference b'ween the player and their nearest enemy", (SInt32)0);

	SME::INI::INISetting			kBattleMusicDieRollChance("ChanceOfPlayback", "Music::CombatPlayback",
															"Chance, in percentage, of successful playback", (SInt32)50);

	SME::INI::INISetting			kBattleMusicStopImmediatelyOnCombatEnd("StopImmediatelyOnCombatEnd", "Music::CombatPlayback",
																		"What he said", (SInt32)1);

	SME::INI::INISetting			kBattleMusicStartPreviousTrackOnCombatEnd("StartPreviousTrackOnCombatEnd", "Music::CombatPlayback",
																		"What he said", (SInt32)1);

	SME::INI::INISetting			kBattleMusicRestorePreviousTrackPlaybackPosition("RestorePreviousTrackPlaybackPosition", "Music::CombatPlayback",
																		"What he said", (SInt32)0);



	SME::INI::INISetting			kMusicQueueImmediatelyOnCellChange("QueueImmediatelyOnCellChange", "Music::General",
																	"Immediately change the current playing track to match the destination cell's music type", (SInt32)1);
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
	
	RegisterSetting(&Settings::kMusicQueueImmediatelyOnCellChange);

	Save();
}

namespace PlayerCombatState
{
	bool CombatFinish = false;
	bool CombatStart = false;
	bool StateChanged = false;

	Actor* LastKnownAttacker = NULL;

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