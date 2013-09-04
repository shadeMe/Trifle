#include "Sundries.h"

namespace Sundries
{
	_DefineHookHdlr(TESDataHandlerPopulatePluginList, 0x0044A3D4);
	_DefinePatchHdlr(PlayerCharacterOnHealthDamage, 0x00A73DC4);


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

	UInt32 __stdcall DoPlayerCharacterOnHealthDamageHook(PlayerCharacter* PC, Actor* Attacker, float Damage)
	{
		PlayerCombatState::LastKnownAttacker = Attacker;

	#ifndef NDEBUG
		if (Attacker)
			_MESSAGE("Actor %s dealt the player %f points of health damage", Attacker->GetFullName()->name.m_data, Damage);
	#endif // !NDEBUG

		return thisCall<UInt32>(0x0065D6F0, PC, Attacker, Damage);
	}

	#define _hhName	PlayerCharacterOnHealthDamage
	_hhBegin()
	{
		__asm
		{	
			mov		eax, [esp + 0x4]
			mov		edx, [esp + 0x8]
			push	edx
			push	eax
			push	ecx
			call	DoPlayerCharacterOnHealthDamageHook
			retn	0x8
		}
	}


	void Patch(void)
	{
		_MemHdlr(TESDataHandlerPopulatePluginList).WriteJump();
		_MemHdlr(PlayerCharacterOnHealthDamage).WriteUInt32((UInt32)&PlayerCharacterOnHealthDamageHook);
	}
}
