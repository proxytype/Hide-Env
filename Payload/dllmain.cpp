// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include <winternl.h>
#include "detours.h"
#include <string>


#define STATUS_INVALID_HANDLE (0xC0000008)
#define BUFSIZE 4096


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

typedef NTSTATUS (WINAPI* realRtlQueryEnvironmentVariable)(
    PVOID Environment, PWSTR Name, size_t NameLength, PWSTR Value, size_t ValueLength, PSIZE_T ReturnLength
);


typedef DWORD(WINAPI* realExpandEnvironmentStringsW)(
    LPCWSTR lpSrc,
    LPWSTR  lpDst,
    DWORD   nSize
);

typedef DWORD(WINAPI* realGetEnvironmentVariableW)(
    LPCWSTR lpName,
    LPWSTR  lpBuffer,
    DWORD   nSize);


realRegEnumValueW _realRegEnumValueW = (realRegEnumValueW)GetProcAddress(GetModuleHandle(L"kernelbase.dll"), "RegEnumValueW");

realRtlQueryEnvironmentVariable _realRtlQueryEnvironmentVariable = (realRtlQueryEnvironmentVariable)GetProcAddress(GetModuleHandle(L"ntdll.dll"),
    "RtlQueryEnvironmentVariable");

realExpandEnvironmentStringsW _realExpandEnvironmentStringsW = (realExpandEnvironmentStringsW)GetProcAddress(GetModuleHandle(L"kernelbase.dll"),
    "ExpandEnvironmentStringsW");

realGetEnvironmentVariableW _realGetEnvironmentVariable = (realGetEnvironmentVariableW)GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "GetEnvironmentVariableW");

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

DWORD _GetEnvironmentVariable(LPCWSTR lpName,
    LPWSTR  lpBuffer,
    DWORD   nSize)
{
    

   DWORD d = _realGetEnvironmentVariable(lpName, lpBuffer, nSize);
   
   OutputDebugString(lpName);
   OutputDebugString(lpBuffer);

   return d;
}

DWORD  _ExpandEnvironmentStringsW(
    LPCWSTR lpSrc,
    LPWSTR  lpDst,
    DWORD   nSize) {


    OutputDebugString(lpDst);
    OutputDebugString(lpSrc);

    DWORD d = 0;
    d = _realExpandEnvironmentStringsW(lpSrc, lpDst, nSize);
    
    //std::wstring ws(lpDst);
    //std::wstring pattern(L"ToHide");
    //std::string::size_type i = ws.find(pattern);

    //if (i != std::wstring::npos) {
    //    OutputDebugString(lpSrc);
    //    OutputDebugString(lpDst);
    //    //lpSrc = {};
    //    d = _realExpandEnvironmentStringsW(L"", lpDst, nSize);
    //} else {
    //   
    //}

    return d;
}

NTSTATUS _RtlQueryEnvironmentVariable(
    PVOID Environment, PWSTR Name, size_t NameLength, PWSTR Value, size_t ValueLength, PSIZE_T ReturnLength)
{

    OutputDebugString(Name);
    OutputDebugString(Value);

    NTSTATUS status = _realRtlQueryEnvironmentVariable(Environment, Name, NameLength, Value, ValueLength, ReturnLength);

    //std::wstring ws(Value);
    //std::wstring pattern(L"ToHide");
    //OutputDebugString(Name);
    //OutputDebugString(Value);
    //std::string::size_type i = ws.find(pattern);

    //if (i != std::wstring::npos) {
    //    OutputDebugString(Value);
    //}

   
    return status;
}

void attachDetours() {

    RegDisablePredefinedCache();
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    char buffer[100];
    sprintf_s(buffer, "check it out: %02X\n", _realExpandEnvironmentStringsW);
    OutputDebugStringA(buffer);

    //DetourAttach((PVOID*)&_realExpandEnvironmentStringsW, _ExpandEnvironmentStringsW);
    //DetourAttach((PVOID*)&_realRtlQueryEnvironmentVariable, _RtlQueryEnvironmentVariable);
    //DetourAttach((PVOID*)&_realGetEnvironmentVariable, _GetEnvironmentVariable);

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