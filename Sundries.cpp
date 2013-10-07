#include "Sundries.h"

namespace Sundries
{
	_DefineHookHdlr(TESDataHandlerPopulatePluginList, 0x0044A3D4);
	_DefineHookHdlr(ActorOnHealthDamage, 0x006034B0);
	_DefineHookHdlr(PlayerFirstPersonShadow, 0x00407684);
	_DefineHookHdlr(TESRender3DWorldShadows, 0x0040C91B);


	bool __stdcall FixPluginListPopulation(WIN32_FIND_DATA* FileData)
	{
		static std::list<std::string> kActivePluginList;
		if (kActivePluginList.size() == 0)
		{
			const char* kAppDataPath = (const char*)0x00B3F178;
			const char* kPluginListName = ".\\Plugins.txt";

			char Buffer[0x104] = {0};
			strcpy_s(Buffer, sizeof(Buffer), kAppDataPath);
			strcat_s(Buffer, sizeof(Buffer), kPluginListName);

			std::fstream PluginListStream(Buffer, std::ios::in);
			if (PluginListStream.fail() == false)
			{
				char Entry[0x200] = {0};
				while (PluginListStream.eof() == false)
				{
					ZeroMemory(Entry, sizeof(Entry));
					PluginListStream.getline(Entry, sizeof(Entry));

					if (strlen(Entry) > 2 && Entry[0] != '#')
					{
						kActivePluginList.push_back(Entry);
					}
				}
			}
		}

		if (!_stricmp(FileData->cFileName, "Oblivion.esm"))
		{
			return true;
		}

		for (std::list<std::string>::const_iterator Itr = kActivePluginList.begin(); Itr != kActivePluginList.end(); Itr++)
		{
			if (!_stricmp(Itr->c_str(), FileData->cFileName))
				return true;
		}

		return false;
	}

	#define _hhName	TESDataHandlerPopulatePluginList
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0044A3DA);
		_hhSetVar(Jump, 0x0044A3E4);
		_hhSetVar(Skip, 0x0044A514);
		__asm
		{
			lea     eax, [esp + 0x2C]

			pushad
			push	eax
			call	FixPluginListPopulation
			test	al, al
			jz		SKIPPLUGIN

			popad
			cmp     [esp + 0x48], ebx
			jnz		AWAY
			jmp		_hhGetVar(Retn)
		SKIPPLUGIN:
			popad
			jmp		_hhGetVar(Skip)
		AWAY:
			jmp		_hhGetVar(Jump)
		}
	}

	void __stdcall DoActorOnHealthDamageHook(Actor* Attacked, Actor* Attacker, float Damage)
	{
		PlayerCombatState::LastKnownAttackee = Attacked;
		PlayerCombatState::LastKnownAttacker = Attacker;

	#ifndef NDEBUG
		if (Attacker && Attacked)
			_MESSAGE("Actor %s dealt actor %s %f points of health damage", Attacker->GetFullName()->name.m_data, Attacked->GetFullName()->name.m_data, Damage);
	#endif // !NDEBUG
	}

	#define _hhName	ActorOnHealthDamage
	_hhBegin()
	{
		_hhSetVar(Retn, 0x006034BB);
		__asm
		{	
			mov		eax, [esp + 0x4]
			mov		edx, [esp + 0x8]
			pushad
			push	edx
			push	eax
			push	ecx
			call	DoActorOnHealthDamageHook
			popad

			push	esi
			mov		esi, ecx
			mov		eax, [esi]
			mov		edx, [eax + 0x198]

			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall EnumeratePC1PShadows(ShadowSceneNode* SceneRoot)
	{
		if (Settings::kGraphicsEnablePlayerFirstPersonShadow.GetData().i)
		{
			if ((*g_thePlayer)->IsThirdPerson() == false &&
				*((UInt8*)0x00B3BB04) == 0)		// vanity cam mode
			{
				TESObjectREFR* Horse = thisVirtualCall<TESObjectREFR*>(0x380, *g_thePlayer);
				UInt32 Refraction = thisCall<UInt32>(0x005E9670, *g_thePlayer);
				UInt32 Invisibility = thisVirtualCall<UInt32>(0x284, *g_thePlayer, kActorVal_Invisibility);

				if (Horse == NULL &&		// when not on horseback
					Refraction == 0 &&		// zero refraction
					Invisibility == 0)		// zero invisibility
				{
					NiNode* ThirdPersonNode = thisCall<NiNode*>(0x00660110, *g_thePlayer, false);
					if (ThirdPersonNode)
					{
						thisCall<UInt32>(0x007C6C30, SceneRoot, ThirdPersonNode);
					}
				}
			}
		}
	}

	#define _hhName	PlayerFirstPersonShadow
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00407689);
		_hhSetVar(Call, 0x007C6DE0);
		__asm
		{				
			pushad
			push	ecx
			call	EnumeratePC1PShadows
			popad


			call	_hhGetVar(Call)
			jmp		_hhGetVar(Retn)
		}
	}

	void __stdcall TogglePC3PNode(bool State)
	{
		if (Settings::kGraphicsEnablePlayerFirstPersonShadow.GetData().i)
		{
			if ((*g_thePlayer)->IsThirdPerson() == false &&
				*((UInt8*)0x00B3BB04) == 0)
			{
				NiNode* ThirdPersonNode = thisCall<NiNode*>(0x00660110, *g_thePlayer, false);
				if (ThirdPersonNode)
				{
					if (State)
						ThirdPersonNode->m_flags |= NiAVObject::kFlag_AppCulled;
					else
						ThirdPersonNode->m_flags &= ~NiAVObject::kFlag_AppCulled;
				}
			}
		}
	}

	#define _hhName	TESRender3DWorldShadows
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040C920);
		_hhSetVar(Call, 0x004073D0);
		__asm
		{				
			pushad
			push	0
			call	TogglePC3PNode
			popad

			call	_hhGetVar(Call)

			pushad
			push	1
			call	TogglePC3PNode
			popad

			jmp		_hhGetVar(Retn)
		}
	}


	void Patch(void)
	{
		_MemHdlr(TESDataHandlerPopulatePluginList).WriteJump();
		_MemHdlr(ActorOnHealthDamage).WriteJump();
		_MemHdlr(PlayerFirstPersonShadow).WriteJump();
		_MemHdlr(TESRender3DWorldShadows).WriteJump();
	}

	void FixHorseCorpseJittering( void )
	{
		if (Settings::kBugFixHorseCorpseCollision.GetData().i == 0)
			return;

		for (TESBoundObject* Itr = (*g_dataHandler)->boundObjects->first; Itr; Itr = Itr->next)
		{
			if (Itr->typeID == kFormType_Creature)
			{
				TESCreature* Creature = OBLIVION_CAST(Itr, TESBoundObject, TESCreature);
				if (Creature)
				{
					if (Creature->type == TESCreature::eCreatureType_Horse && Creature->actorBaseData.CanCorpseCheck())
					{
#ifndef NDEBUG
						if (Creature->GetFullName()->name.m_dataLen)
							_MESSAGE("Removing corpse check flag from horse %s %08X", Creature->GetFullName()->name.m_data, Creature->refID);
#endif // !NDEBUG
						Creature->actorBaseData.SetCanCorpseCheck(false);
					}
				}
			}
		}
	}

}
