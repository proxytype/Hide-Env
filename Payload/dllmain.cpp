// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include <winternl.h>
#include "detours.h"
#include <string>

#define STATUS_INVALID_HANDLE (0xC0000008)

typedef LSTATUS(WINAPI* realRegEnumValueW)(
    HKEY    hKey,
    DWORD   dwIndex,
    LPWSTR  lpValueName,
    LPDWORD lpcchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    );


realRegEnumValueW _realRegEnumValueW = (realRegEnumValueW)GetProcAddress(GetModuleHandle(L"kernelbase.dll"), "RegEnumValueW");

LSTATUS _RegEnumValueW(
    HKEY    hKey,
    DWORD   dwIndex,
    LPWSTR  lpValueName,
    LPDWORD lpcchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
) {
    LSTATUS status = _realRegEnumValueW(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);

    std::wstring s1(lpValueName);
    //the name of the variable to remove
    if (s1.compare(L"ToHide") == 0) {
        return STATUS_INVALID_HANDLE;
    }

    return status;
}

void attachDetours() {

    RegDisablePredefinedCache();
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach((PVOID*)&_realRegEnumValueW, _RegEnumValueW);

    DetourTransactionCommit();
}

void deAttachDetours() {

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourDetach((PVOID*)&_realRegEnumValueW, _RegEnumValueW);

    DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        attachDetours();
        break;
    case DLL_PROCESS_DETACH:
        deAttachDetours();
        break;
    }
    return TRUE;
}