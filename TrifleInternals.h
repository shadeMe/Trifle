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

#include <dsound.h>

#include <SME_Prefix.h>
#include <MemoryHandler.h>
#include <INIManager.h>
#include <StringHelpers.h>
#include <MersenneTwister.h>


using namespace SME;
using namespace SME::MemoryHandler;

namespace Interfaces
{
	extern PluginHandle						kOBSEPluginHandle;
}

class TrifleINIManager : public INI::INIManager
{
public:
	void								Initialize(const char* INIPath, void* Parameter);

	static TrifleINIManager			Instance;
};

namespace Settings
{
	extern SME::INI::INISetting				kBattleMusicPlaybackMode;
}

_DeclareMemHdlr(SoundManagerPlayBatleMusic, "sloblock to battle music");
_DeclareMemHdlr(SoundManagerMusicPlayback, "");
_DeclareMemHdlr(TESDataHandlerPopulatePluginList, "fixes the plugin list init code to skip inactive files");

void CloseTheLoop(void);
