
#include <iostream>
#include <windows.h>
#include <TlHelp32.h>
#include "mem.h"
#include "proc.h"
#include "ent.h"

typedef ent* ( __cdecl* tGetCrosshairEnt )();
tGetCrosshairEnt g_GetCrosshairEnt = nullptr;

float g_elevatedHeight = std::numeric_limits<float>::infinity();
float g_deltaHeight = 10.0f;
DWORD g_jmpBackAddr;
char* g_username = nullptr;

// no prolog/epilog
void __declspec(naked) Levitate() {
	__asm {
		fstp dword ptr[esi + 0x3c]

		; compare g_username with the username in the struct
		push eax
		push esi
		push edi
		
		lea edi, [esi + 0x225]
		mov esi, g_username
		push esi
		push edi
		call strcmp
		add esp, 8
		cmp eax, 0
		jne done

		pop edi
		pop esi
		pop eax




		; save reg
		push eax

		; +inf in int format
		xor eax, eax
		mov eax, 0xff
		shl eax, 23

		cmp dword ptr [g_elevatedHeight], eax
		
		; save reg
		pop eax

		jne cont

		fld dword ptr[esi+0x3c]
		fadd dword ptr[g_deltaHeight]
		fstp dword ptr[g_elevatedHeight]

	cont:
		fld dword ptr[g_elevatedHeight]
		fstp dword ptr [esi+0x3c]
		

		; overwritten instructions from the jump
		pop edi
		pop ebp

		jmp[g_jmpBackAddr];


	done:
		pop edi
		pop esi
		pop eax

		pop edi
		pop ebp
		jmp[g_jmpBackAddr]
	}
}

DWORD WINAPI HackThread(HMODULE hModule) {
	AllocConsole();
	
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	std::cout << "Started client..." << std::endl;

	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"ac_client.exe");
	g_GetCrosshairEnt = (tGetCrosshairEnt) (moduleBase + 0x607c0);

	bool bHealth = false, bAmmo = false, bRecoil = false, bLevitate = false, bTriggerbot = false;

	ent* localPlayerPtr{ nullptr };

	while (true) {

		if (GetAsyncKeyState(VK_END) & 1) {
			break;
		}

		if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
			bHealth = !bHealth;
		}
		
		if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
			bAmmo = !bAmmo;
		}
		
		if (GetAsyncKeyState(VK_NUMPAD3) & 1) {
			bRecoil = !bRecoil;
			if (bRecoil) {
				mem::Nop((BYTE*)(moduleBase + 0x63786), 10);
			}
			else {
				// original instructions
				mem::Patch((BYTE*)(moduleBase + 0x63786), (BYTE*)"\x50\x8d\x4c\x24\x1c\x51\x8b\xce\xff\xd2", 10);
			}
		}

		if (GetAsyncKeyState(VK_NUMPAD4) & 1) {
			bLevitate = !bLevitate;
			
			if (bLevitate) {
				DWORD hookAddr = (DWORD)(moduleBase + 0x5be04);
				int hookLength = 5;
				
				// global
				g_jmpBackAddr = hookAddr + hookLength;

				mem::Hook((void*)hookAddr, Levitate, hookLength);
				
			}
			else {
				// we have to reset the height because player pos height changed
				g_elevatedHeight = std::numeric_limits<float>::infinity();
				
				mem::Patch((BYTE*)(moduleBase + 0x5be04), (BYTE*)"\xd9\x5e\x3c\x5f\x5d", 5);
			} 
		}

		if (GetAsyncKeyState(VK_NUMPAD5) & 1) {
			bTriggerbot = !bTriggerbot;
		}

		localPlayerPtr = *(ent**)(moduleBase + 0x10f4f4);
		
		if (localPlayerPtr) {
			g_username = localPlayerPtr->username;

			if (bHealth) {
				localPlayerPtr->health = 1337;
			}

			if (bAmmo) {
				localPlayerPtr->currentWeapon->ammoClip->ammo = 1337;
			}

			if (bTriggerbot) { 
				ent* crosshairEnt = g_GetCrosshairEnt();

				if (crosshairEnt) {
					
					if (localPlayerPtr->team != crosshairEnt->team) {
						localPlayerPtr->bAttack = 1;
					}

				}
				else {
					localPlayerPtr->bAttack = 0;
				}
			}
		}

		Sleep(5);
	}

	fclose(f);
	FreeConsole();
	FreeLibraryAndExitThread(hModule, 0);
	return 0;

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

int main() {
	return 0;
}
