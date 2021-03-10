#pragma once

#include "TrifleInternals.h"

namespace Sundries
{
	_DeclareMemHdlr(TESDataHandlerPopulatePluginList, "fixes the plugin list init code to skip inactive files");
	_DeclareMemHdlr(ActorOnHealthDamage, "allows the detection of an actor's health deduction");
	_DeclareMemHdlr(DialogueMenuSpeakResponse, "plays the NPC's dialogue with locational/3D information");

	void Patch(void);

	void FixHorseCorpseJittering(void);
}

