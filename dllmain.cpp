#pragma once
#define RELATIVE_ADDR(addr, size) ((PBYTE)((UINT_PTR)(addr) + *(PINT)((UINT_PTR)(addr) + ((size) - sizeof(INT))) + (size)))
#include <winscard.h>
#include <windows.h>
#include <cstdint>
#include <Psapi.h>
#include <vector>
#include <locale>
#include <String>
#include <d3d11.h>
#include <iostream>
#include <fstream>
#include "minhook/MinHook.h"
#include "structs.h"

uint64_t base_address;
uintptr_t GetObjectNames;
uintptr_t FreeFN;
uintptr_t ProjectWorldToScreen;

uintptr_t LocalPawn;
uintptr_t Localplayer;
uintptr_t PlayerController;
uintptr_t Ulevel;
uintptr_t Levels;

uintptr_t OFFSET_OwningGameInstance = 0x0;
uintptr_t OFFSET_LocalPlayers = 0x0;
uintptr_t OFFSET_PlayerController = 0x0;
uintptr_t OFFSET_AcknowledgedPawn = 0x0;
uintptr_t OFFSET_ROOTCOMPONENT = 0x0;
uintptr_t OFFSET_AACTORS = 0x0;
uintptr_t OFFSET_ACTORCOUNT = 0x0;
uintptr_t OFFSET_Levels = 0x0;
uintptr_t OFFSET_PersistentLevel = 0x0;
uintptr_t OFFSET_RELATIVELOCATION = 0x0;

template<typename T>
T read(DWORD_PTR address, const T& def = T())
{
	return *(T*)address;
}

BOOLEAN MaskCompare(PVOID buffer, LPCSTR pattern, LPCSTR mask) {
	for (auto b = reinterpret_cast<PBYTE>(buffer); *mask; ++pattern, ++mask, ++b) {
		if (*mask == 'x' && *reinterpret_cast<LPCBYTE>(pattern) != *b) {
			return FALSE;
		}
	}

	return TRUE;
}

PBYTE FindPattern(PVOID base, DWORD size, LPCSTR pattern, LPCSTR mask) {
	size -= static_cast<DWORD>(strlen(mask));

	for (auto i = 0UL; i < size; ++i) {
		auto addr = reinterpret_cast<PBYTE>(base) + i;
		if (MaskCompare(addr, pattern, mask)) {
			return addr;
		}
	}

	return NULL;
}
PBYTE FindPattern(LPCSTR pattern, LPCSTR mask) {
	MODULEINFO info = { 0 };
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(0), &info, sizeof(info));

	return FindPattern(info.lpBaseOfDll, info.SizeOfImage, pattern, mask);
}

template <typename Ret, typename... Args>
Ret SpoofCall(Ret(*fn)(Args...), Args... args) {
	static PVOID trampoline = nullptr;
	if (!trampoline) {
		trampoline = FindPattern("\xFF\x27", "xx");
		if (!trampoline) {
			MessageBox(0, L"Failed to find valid trampoline", L"Failure", 0);
			ExitProcess(0);
		}
	}

	struct {
		PVOID Trampoline;
		PVOID Function;
		PVOID Reg;
	} params = {
		trampoline,
		reinterpret_cast<void*>(fn),
	};

	return _SpoofCallInternal::Remapper<sizeof...(Args), void>::template Call<Ret, Args...>(&_SpoofCallInternal::RetSpoofStub, &params, args...);
}

void FreeMemory(__int64 address)
{
	if (!address) return;

	auto func = reinterpret_cast<__int64(__fastcall*)(__int64 a1)>(FreeFN);

	SpoofCall(func, address);
}

std::string GetObjectName(uintptr_t Object) { //dano20zombie <3
	if (Object == NULL)
		return xor_("")c_str();
	auto fGetObjName = reinterpret_cast<FString * (__fastcall*)(FString * name, uintptr_t entity)>(GetObjectNames);
	FString result;
	SpoofCall(fGetObjName, &result, Object);
	if (result.c_str() == NULL)
		return xor_("")c_str();

	auto result_str = result.ToString();
	if (result.c_str() != NULL)
		SpoofCall(FreeMemory, (__int64)result.c_str());
	return result_str;
}

bool WorldToScreen(Vector3 vWorldPos, Vector3* vScreenPos)
{
	auto
		WorldToScreen
		=
		reinterpret_cast<bool(__fastcall*)(uintptr_t pPlayerController,
			Vector3 vWorldPos,
			Vector3 * vScreenPosOut,
			char)>(ProjectWorldToScreen);

	return
		SpoofCall(WorldToScreen,
			(uintptr_t)PlayerController,
			vWorldPos, vScreenPos,
			(char)0);
}

void inline Draw(uintptr_t pawn, std::string object_name)
{
	if (!pawn) return;
	uintptr_t root = read<uintptr_t>(pawn + OFFSET_ROOTCOMPONENT);
	if (!root) return;
	Vector3 CenterLocation = *(Vector3*)(root + OFFSET_RELATIVELOCATION);
	Vector3 vScreenPos;
	WorldToScreen(CenterLocation, &vScreenPos);
	if (vScreenPos.x == 0 && vScreenPos.y == 0) return;
	//DrawText(object_name.c_str(), vScreenPos.x, vScreenPos.y, object_name.length(), color);
	//use your choice
}

void actorloop()
{
	if (!LocalPawn) return;

	uintptr_t AActors = read<uintptr_t>(Ulevel + OFFSET_AACTORS);
	uintptr_t ActorCount = *(int*)(Ulevel + OFFSET_ACTORCOUNT);

	for (int count = 0; count < ActorCount; count++)
	{
		uintptr_t CurrentActor = read<uintptr_t>(AActors + count * 0x8);
		if (!CurrentActor) continue;
		std::string ObjName = GetObjectName(CurrentActor);
		if (ObjName == "") continue;
		if (strstr(ObjName.c_str(), xor_("PlayerPawn_Athena_C").c_str()))
		{
			Draw(CurrentActor, xor_("[PLAYER]").c_str());
		}
	}

}

bool updateaddr()//add this to your draw loop whatever me good now :D and it should work. 
{
	MH_Initialize();
	uintptr_t uworld_r;
	if (!uworld_r)
	{
		uworld_r = (uintptr_t)FindPattern(xor_("\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\xDB\x74\x3B\x41").c_str(), xor_("xxx????xxxxxx").c_str());
		uworld_r = reinterpret_cast<uintptr_t>(RELATIVE_ADDR(uworld_r, 7));
		if (!uworld_r)
		{
			MessageBoxA(0, xor_("UWORLD FAILED").c_str(), 0, 0);
		}
	}

	if (!GetObjectNames)
	{
		GetObjectNames = (uintptr_t)FindPattern(xor_("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x85\xD2\x75\x45\x33\xC0\x48\x89\x01\x48\x89\x41\x08\x8D\x50\x05\xE8\x00\x00\x00\x00\x8B\x53\x08\x8D\x42\x05\x89\x43\x08\x3B\x43\x0C\x7E\x08\x48\x8B\xCB\xE8\x00\x00\x00\x00\x48\x8B\x0B\x48\x8D\x15\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xC3\x48\x83\xC4\x20\x5B\xC3\x48\x8B\x42\x18").c_str(), xor_("xxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxx????xxxxxx????xx????x????xxxxxxxxxxxxx").c_str());
		if (!GetObjectNames)
		{
			MessageBoxA(0, xor_("GETOBJECTNAMES FAILED").c_str(), 0, 0);
		}
	}

	if (!FreeFN)
	{
		FreeFN = (uintptr_t)FindPattern(xor_("\x48\x85\xC9\x74\x2E\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x75\x0C").c_str(), xor_("xxxxxxxxxxxxxxxx????xxxxx").c_str());
		if (!FreeFN)
		{
			MessageBoxA(0, xor_("FREE FAILED"), 0, 0);
		}
	}

	if (!ProjectWorldToScreen)
	{
		ProjectWorldToScreen = (uintptr_t)FindPattern(xor_("\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x83\xC4\x28\xC3\xCC\x48\xE9\xCC\x2F\xC2\xFB").c_str(), xor_("xxxxx????xxxxxxxxxxxx").c_str());
		if (!ProjectWorldToScreen)
		{
			MessageBoxA(0, xor_("ProjectWorldToScreen FAILED"), 0, 0);
		}
	}

	uintptr_t GWorld = read<uintptr_t>(uworld_r);
	if (!GWorld) return false;

	uintptr_t Gameinstance = read<uintptr_t>(GWorld + OFFSET_OwningGameInstance);
	if (!Gameinstance) return false;

	uintptr_t LocalPlayers = read<uintptr_t>(Gameinstance + OFFSET_LocalPlayers);
	if (!LocalPlayers) return false;

	Localplayer = read<uintptr_t>(LocalPlayers);
	if (!Localplayer) return false;

	PlayerController = read<uintptr_t>(Localplayer + OFFSET_PlayerController);
	if (!PlayerController) return false;

	LocalPawn = read<uint64_t>(PlayerController + OFFSET_AcknowledgedPawn);
	if (!LocalPawn)
		return false;

	Levels = read<uintptr_t>(GWorld + OFFSET_Levels);
	if (!Levels) return false;

	OFFSET_ACTORCOUNT = read<uintptr_t>(GWorld + OFFSET_Levels);
	if (!OFFSET_ACTORCOUNT) OFFSET_ACTORCOUNT = read<uintptr_t>(GWorld + OFFSET_Levels + 0x8);
	if (!OFFSET_ACTORCOUNT) return false;

	Ulevel = read<uintptr_t>(GWorld + OFFSET_PersistentLevel);
	if (!Ulevel) return false;

	actorloop();
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		base_address = (uint64_t)GetModuleHandleW(NULL);
	}
}