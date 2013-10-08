#include "Graphics.h"

namespace Graphics
{
	_DefineHookHdlr(PlayerFirstPersonShadow, 0x00407684);
	_DefineHookHdlr(TESRender3DWorldShadows, 0x0040C91B);
	_DefineHookHdlr(PlayerFirstPersonCamera, 0x0040D658);
	_DefineHookHdlr(FirstPersonCameraNodeSwap1, 0x0058072C);
	_DefineHookHdlr(FirstPersonCameraNodeSwap2, 0x006604D5);
	_DefineHookHdlr(FirstPersonCameraNodeSwap3, 0x005F1208);
	_DefineHookHdlr(FirstPersonCameraNodeSwap4, 0x0066C798);
	_DefineHookHdlr(FirstPersonCameraNodeSwap5, 0x006055C8);
	_DefineHookHdlr(FirstPersonCameraNodeSwap6, 0x0066B8D6);
	_DefineHookHdlr(FirstPersonCameraNodeSwap7, 0x0066B92B);
	_DefineHookHdlr(FirstPersonCameraNodeSwap8, 0x0066BA02);
	_DefineHookHdlr(FirstPersonCameraNodeSwap9, 0x0066BA23);
	
	static NiNode** kFirstPersonCameraNode = (NiNode**)0x00B3BB0C;

	bool __stdcall GetVanityCamState()
	{
		return *((UInt8*)0x00B3BB04) == 1;
	}

	void __stdcall ToggleFaceGenNode(NiNode* Root, bool State)
	{
		SME_ASSERT(Root);

		static const char* kFaceGenParts[9] = 
		{
			"FaceGenFace",
			"FaceGenHair",
			"FaceGenEars",
			"FaceGenMouth",
			"FaceGenTeethLower",
			"FaceGenTeethUpper",
			"FaceGenTongue",
			"FaceGenEyeLeft",
			"FaceGenEyeRight"
		};

		for (int i = 0; i < 9; i++)
		{
			NiAVObject* Mesh = cdeclCall<NiAVObject*>(0x006F94A0, Root, kFaceGenParts[i]);
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
					if (Settings::kGraphicsEnablePlayerFirstPersonBody.GetData().i == 0)
						ThirdPersonNode->m_flags |= NiAVObject::kFlag_AppCulled;
					else
						ToggleFaceGenNode(ThirdPersonNode, false);
				}
				else
				{
					if (Settings::kGraphicsEnablePlayerFirstPersonBody.GetData().i == 0)
						ThirdPersonNode->m_flags &= ~NiAVObject::kFlag_AppCulled;
					else
						ToggleFaceGenNode(ThirdPersonNode, true);
				}
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

	void __stdcall MessWithSceneCamera(bool State)
	{
		SME_ASSERT((*g_worldSceneGraph)->m_children.numObjs > 0);

		NiNode* WorldCameraRoot = (NiNode*)((SceneGraph*)(*g_worldSceneGraph))->m_children.data[0];
		NiCamera* WorldCamera = ((SceneGraph*)(*g_worldSceneGraph))->camera;

		NiMatrix33* CameraRootWorldRotate = &WorldCameraRoot->m_worldRotate;
		NiMatrix33* CameraRootLocalRotate = &WorldCameraRoot->m_localRotate;

		Vector3* CameraRootWorldTranslate = (Vector3*)&WorldCameraRoot->m_worldTranslate;
		Vector3* CameraRootLocalTranslate = (Vector3*)&WorldCameraRoot->m_localTranslate;
		
		if ((*g_thePlayer)->IsThirdPerson() == false && *((UInt8*)0x00B3BB04) == 0)			
		{
			NiNode* ThirdPersonNode = thisCall<NiNode*>(0x00660110, *g_thePlayer, false);
			NiNode* FirstPersonNode = thisCall<NiNode*>(0x00660110, *g_thePlayer, true);
			
			if (ThirdPersonNode && FirstPersonNode)
			{
				if (ThirdPersonNode->m_children.numObjs >= 6)
				{
					NiNode* FaceGenNode = (NiNode*)ThirdPersonNode->m_children.data[5];
					if (FaceGenNode)
					{
						FirstPersonNode->m_flags |= NiAVObject::kFlag_AppCulled;
						ThirdPersonNode->m_flags &= ~NiAVObject::kFlag_AppCulled;

						if (*((UInt8*)((UInt32)*g_thePlayer) + 0x748) == 0)		// fly cam disabled
						{
							if (State)
							{			
								TODO("Fix crosshair and y-axis clamping")

								CameraRootLocalTranslate->x = FaceGenNode->m_kWorldBound.x;
								CameraRootLocalTranslate->y = FaceGenNode->m_kWorldBound.y;
								CameraRootLocalTranslate->z = FaceGenNode->m_kWorldBound.z + FaceGenNode->m_kWorldBound.radius;

								thisCall<void>(0x00707370, WorldCameraRoot, 0.0, 1);

								ToggleFaceGenNode(ThirdPersonNode, false);
							}
							else
							{
								ToggleFaceGenNode(ThirdPersonNode, true);
							}
						}
					}
				}
			}
		}
	}

	#define _hhName	PlayerFirstPersonCamera
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040D65D);
		_hhSetVar(Call, 0x0040C830);
		__asm
		{				
			pushad
			push	1
			call	MessWithSceneCamera
			popad

			call	_hhGetVar(Call)

			pushad
			push	0
			call	MessWithSceneCamera
			popad

			jmp		_hhGetVar(Retn)
		}
	}
	
	#define _hhName	FirstPersonCameraNodeSwap1
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00580736);
		__asm
		{				
			call	GetWorldCameraRoot
			mov     ecx, [eax + 0x88]
			mov     [esp + 0x20], ecx
			jmp		_hhGetVar(Retn)
		}
	}
	
	#define _hhName	FirstPersonCameraNodeSwap2
	_hhBegin()
	{
		_hhSetVar(Retn, 0x006604DB);
		__asm
		{				
			call	GetWorldCameraRoot
			mov     ecx, [eax + 0x88]
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	FirstPersonCameraNodeSwap3
	_hhBegin()
	{
		_hhSetVar(Retn, 0x005F120E);
		__asm
		{				
			call	GetWorldCameraRoot
			mov     edx, [eax + 0x88]
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	FirstPersonCameraNodeSwap4
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0066C79E);
		__asm
		{				
			call	GetWorldCameraRoot
			mov     edx, [eax + 0x8C]
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	FirstPersonCameraNodeSwap5
	_hhBegin()
	{
		_hhSetVar(Retn, 0x006055CE);
		__asm
		{				
			call	GetWorldCameraRoot
			mov     ecx, [eax + 0x88]
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	FirstPersonCameraNodeSwap6
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0066B8DC);
		__asm
		{				
			call	GetWorldCameraRoot
			mov     ecx, [eax + 0x88]
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	FirstPersonCameraNodeSwap7
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0066B930);
		__asm
		{				
			call	GetWorldCameraRoot
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	FirstPersonCameraNodeSwap8
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0066BA07);
		__asm
		{				
			call	GetWorldCameraRoot
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	FirstPersonCameraNodeSwap9
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0066BA29);
		__asm
		{				
			call	GetWorldCameraRoot
			mov		eax, ecx
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

		if (Settings::kGraphicsEnablePlayerFirstPersonBody.GetData().i)
		{
			_MemHdlr(PlayerFirstPersonCamera).WriteJump();		
			_MemHdlr(FirstPersonCameraNodeSwap1).WriteJump();
			_MemHdlr(FirstPersonCameraNodeSwap2).WriteJump();
			_MemHdlr(FirstPersonCameraNodeSwap3).WriteJump();
			_MemHdlr(FirstPersonCameraNodeSwap4).WriteJump();
			_MemHdlr(FirstPersonCameraNodeSwap5).WriteJump();
			_MemHdlr(FirstPersonCameraNodeSwap6).WriteJump();
			_MemHdlr(FirstPersonCameraNodeSwap7).WriteJump();
			_MemHdlr(FirstPersonCameraNodeSwap8).WriteJump();
			_MemHdlr(FirstPersonCameraNodeSwap9).WriteJump();
		}
	}
}
