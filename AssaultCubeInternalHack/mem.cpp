#include "mem.h"

void mem::PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess) {
	DWORD oldProtect;
	VirtualProtectEx(hProcess, dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	WriteProcessMemory(hProcess, dst, src, size, nullptr);
	VirtualProtectEx(hProcess, dst, size, oldProtect, &oldProtect);
}

void mem::NopEx(BYTE* dst, unsigned int size, HANDLE hProcess) {
	BYTE* nopArray = new BYTE[size];
	memset(nopArray, 0x90, size);

	PatchEx(dst, nopArray, size, hProcess);
	delete[] nopArray;
}

uintptr_t mem::FindDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets) {
	uintptr_t addr = ptr;

	for (unsigned int i = 0; i < offsets.size(); i++) {
		ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), nullptr);
		addr += offsets[i];
	}

	return addr;
}

void mem::Patch(BYTE* dst, BYTE* src, unsigned int size) {
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(dst, src, size);
	VirtualProtect(dst, size, oldProtect, &oldProtect);
}

void mem::Nop(BYTE* dst, unsigned int size) {
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memset(dst, 0x90, size);
	VirtualProtect(dst, size, oldProtect, &oldProtect);
}

uintptr_t mem::FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets) {
	uintptr_t addr = ptr;

	for (unsigned int i = 0; i < offsets.size(); i++) {
		addr = *(uintptr_t*)addr;
		addr += offsets[i];
	}

	return addr;
}

// hook functions
bool mem::Hook(void* toHook, void* ourFunc, int len) {
	if (len < 5)
		return false;

	DWORD prot;
	VirtualProtect(toHook, len, PAGE_EXECUTE_READWRITE, &prot);

	memset(toHook, 0x90, len);
	
	DWORD relativeAddr = ((DWORD)ourFunc) - ((DWORD)toHook + 5);

	// jmp
	*(BYTE*)toHook = 0xe9;
	*(DWORD*)((DWORD)toHook + 1) = relativeAddr;

	DWORD tmp;
	VirtualProtect(toHook, len, prot, &tmp);

	return true;
}

bool mem::Detour32(BYTE* src, BYTE* dst, const uintptr_t len) {
	if (len < 5)
		return false;

	DWORD prot;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &prot);

	uintptr_t relativeAddr = dst - src - 5;
	*src = 0xe9;
	*(uintptr_t*)(src + 1) = relativeAddr;

	VirtualProtect(src, len, prot, &prot);
	return true;
}

BYTE* mem::TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len) {
	if (len < 5)
		return false;

	BYTE* gateway = (BYTE*)VirtualAlloc(0, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memcpy_s(gateway, len, src, len);

	// calculate the relative offset to the original function
	uintptr_t gatewayRelativeAddr = src - gateway - 5;
	
	// write jmp instruction
	*(gateway + len) = 0xe9;
	*(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayRelativeAddr;

	Detour32(src, dst, len);
	return gateway;
}
