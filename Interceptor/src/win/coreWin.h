#pragma once
#ifdef IN_PLATFORM_WINDOWS
#include <process.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

namespace platform
{
    int execCommand(char progname[], char option1[]);
    void sleep_t(unsigned int seconds);

    void CreateChildProcess(char prog[], char option1[]);
    void ErrorExit(const wchar_t* lpszFunction);

    std::string execCtrlCmds(std::vector<std::string> cmdArgs);
    void CreateCtrlChildProcess(std::string);
    std::vector<std::string> ReadFromPipe(void);
    LSTATUS ReadRegistry(LPCWSTR sPath, LPCWSTR sKey, LPWSTR pBuffer, LPDWORD pBufferSize);
}

#endif