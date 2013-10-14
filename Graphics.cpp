#include "Graphics.h"

namespace Graphics
{
	_DefineHookHdlr(PlayerFirstPersonShadow, 0x00407684);
	_DefineHookHdlr(TESRender3DWorldShadows, 0x0040C91B);
	
	bool __stdcall GetVanityCamState()
	{
		return *((UInt8*)0x00B3BB04) == 1;
	}

	NiObjectNET* __stdcall LookupNiObjectByName(NiObjectNET* Source, const char* Name)
	{
		return cdeclCall<NiObjectNET*>(0x006F94A0, Source, Name);
	}

	void __stdcall ToggleFaceGenNode(NiNode* Root, bool State)
	{
		SME_ASSERT(Root);

		static const char* kFaceGenParts[2] = 
		{
			"Bip01 Head",
			"FaceGenFace",
		};

		for (int i = 0; i < 2; i++)
		{
			NiAVObject* Mesh = (NiAVObject*)LookupNiObjectByName(Root, kFaceGenParts[i]);
			if (Mesh)
			{
				if (State == false)
					Mesh->m_flags |= NiAVObject::kFlag_AppCulled;
				else
					Mesh->m_flags &= ~NiAVObject::kFlag_AppCulled;
			}
		}		
	}

	void __stdcall ToggleTorchNode(NiNode* Root, bool State)
	{
		SME_ASSERT(Root);

		static const char* kTorchParts[17] = 
		{
			"FlameNode0",
			"FlameNode1",
			"FlameNode2",
			"FlameNode3",
			"FlameNode4",
			"FlameNode5",
			"FlameNode6",
			"FlameNode7",
			"FlameNode8",
			"FlameNode9",
			"FlameNode10",
			"FlameNode11",
			"FlameNode12",
			"FlameNode13",
			"FlameNode14",
			"FlameNode15",
			"FlameCap",
		};

		for (int i = 0; i < 17; i++)
		{
			NiAVObject* Mesh = (NiAVObject*)LookupNiObjectByName(Root, kTorchParts[i]);
			if (Mesh)
			{
				if (State == false)
					Mesh->m_flags |= NiAVObject::kFlag_AppCulled;
				else
					Mesh->m_flags &= ~NiAVObject::kFlag_AppCulled;
			}
		}		
	}
	
	void __stdcall TogglePC3PNode(bool State)
	{
		if ((*g_thePlayer)->IsThirdPerson() == false && GetVanityCamState() == false)
		{
			NiNode* ThirdPersonNode = thisCall<NiNode*>(0x00660110, *g_thePlayer, false);
			if (ThirdPersonNode)
			{
				if (State == false)
				{
					ThirdPersonNode->m_flags |= NiAVObject::kFlag_AppCulled;
				}
				else
				{
					ThirdPersonNode->m_flags &= ~NiAVObject::kFlag_AppCulled;
				}

				// cull equipped torch flame nodes before rendering the skeleton to the shadow map
				// otherwise, they show up in first person
				ToggleTorchNode(ThirdPersonNode, State == false);
			}
		}
	}

	NiNode* __stdcall GetWorldCameraRoot(void)
	{
		SME_ASSERT((*g_worldSceneGraph)->m_children.numObjs > 0);

		return (NiNode*)((SceneGraph*)(*g_worldSceneGraph))->m_children.data[0];
	}
		
	void __stdcall EnumeratePC1PShadows(ShadowSceneNode* SceneRoot)
	{
		if ((*g_thePlayer)->IsThirdPerson() == false && GetVanityCamState() == false)
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

	#define _hhName	TESRender3DWorldShadows
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040C920);
		_hhSetVar(Call, 0x004073D0);
		__asm
		{				
			pushad
			push	1
			call	TogglePC3PNode
			popad

			call	_hhGetVar(Call)

			pushad
			push	0
			call	TogglePC3PNode
			popad

			jmp		_hhGetVar(Retn)
		}
	}	

	void Patch(void)
	{
		if (Settings::kGraphicsEnablePlayerFirstPersonShadow.GetData().i)
		{
			_MemHdlr(PlayerFirstPersonShadow).WriteJump();
			_MemHdlr(TESRender3DWorldShadows).WriteJump();
		}
	}
}
