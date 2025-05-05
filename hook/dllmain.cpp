// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <winsock2.h>
#include <windows.h>
#include "detours.h"
#include <shlwapi.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "detours.lib")
#pragma warning(disable:4996) 

typedef int (WINAPI *sendto_func) (SOCKET, const char*, int, int, const sockaddr*, int);
typedef int (WINAPI *wsasendto_func) (SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, const sockaddr*, int, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
static sendto_func _sendto = NULL;
static wsasendto_func _wsasendto = NULL;

const int ADDRESS_SIZE = 64;
char address[ADDRESS_SIZE] = { 0 };

bool is_broadcast_ip(const struct in_addr& ip) {
	if (ip.S_un.S_addr == INADDR_BROADCAST)
		return true;
	return false;
}

static int WINAPI fake_sendto(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen)
{
	sockaddr_in* origin_to = (sockaddr_in*)to;

	if (is_broadcast_ip(origin_to->sin_addr)) {
		sockaddr_in to;
		to.sin_family = origin_to->sin_family;
		to.sin_port = origin_to->sin_port;
		to.sin_addr.s_addr = inet_addr(address);
		return _sendto(s, buf, len, flags, (SOCKADDR*)&to, sizeof(to));
	}

	return _sendto(s, buf, len, flags, to, tolen);
}

static int WINAPI fake_wsasendto(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const sockaddr* lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	sockaddr_in* origin_to = (sockaddr_in*)lpTo;

	if (is_broadcast_ip(origin_to->sin_addr)) {
		sockaddr_in to;
		to.sin_family = origin_to->sin_family;
		to.sin_port = origin_to->sin_port;
		to.sin_addr.s_addr = inet_addr(address);
		return _wsasendto(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, (SOCKADDR*)&to, sizeof(to), lpOverlapped, lpCompletionRoutine);
	}

	return _wsasendto(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iTolen, lpOverlapped, lpCompletionRoutine);
}

void InstallHooks()
{
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	_sendto = (sendto_func)DetourFindFunction("ws2_32.dll", "sendto");
	_wsasendto = (wsasendto_func)DetourFindFunction("ws2_32.dll", "WSASendTo");
	if (_sendto != NULL && _wsasendto != NULL)
	{
		DetourAttach(&(PVOID&)_sendto, fake_sendto);
		DetourAttach(&(PVOID&)_wsasendto, fake_wsasendto);
	}

	DetourTransactionCommit();
}

// Ð¶ÔØ¹³×Ó
void UninstallHooks()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (_sendto != NULL && _wsasendto != NULL)
	{
		DetourDetach(&(PVOID&)_sendto, fake_sendto);
		DetourDetach(&(PVOID&)_wsasendto, fake_wsasendto);
	}

	DetourTransactionCommit();
}

BOOL ReadAddress(HMODULE hModule) {
	char szPath[MAX_PATH];
	if (GetModuleFileNameA(hModule, szPath, MAX_PATH) == 0)
		return FALSE;
	PathRemoveFileSpecA(szPath);
	if (!PathAppendA(szPath, "address.txt"))
		return FALSE;
	HANDLE hFile = CreateFileA(szPath, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD dwBytesRead;
	if (!ReadFile(hFile, address, ADDRESS_SIZE - 1, &dwBytesRead, NULL))
		return FALSE;
	if (dwBytesRead > 0 && dwBytesRead <= ADDRESS_SIZE - 1)
		address[dwBytesRead] = '\0';
	else
		return FALSE;
	CloseHandle(hFile);
	return TRUE;
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		if (!ReadAddress(hModule))
			return FALSE;
		InstallHooks();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		UninstallHooks();
		break;
	}
	return TRUE;
}

