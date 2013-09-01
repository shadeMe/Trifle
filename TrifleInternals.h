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

#include <SME_Prefix.h>
#include <MemoryHandler.h>
#include <INIManager.h>
#include <StringHelpers.h>


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
	extern SME::INI::INISetting				kDisableBattleMusic;
}

_DeclareNopHdlr(SoundManagerPlayBatleMusic, "sloblock to battle music");

void CloseTheLoop(void);
