#pragma once

#include "TrifleInternals.h"

namespace Sundries
{
	_DeclareMemHdlr(TESDataHandlerPopulatePluginList, "fixes the plugin list init code to skip inactive files");
	_DeclareMemHdlr(PlayerCharacterOnHealthDamage, "allows the detection of the player's health deduction");

	void Patch(void);
}

