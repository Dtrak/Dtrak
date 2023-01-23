#include "../common.h"
#include "coreWin.h"

#ifdef IN_PLATFORM_WINDOWS

namespace platform
{
    //int execCommand(char progname[], char option1[]){
    //    int result;
    //    result = _spawnl(_P_NOWAITO, progname, progname, option1, NULL);
    //    if (result != -1)
    //        return 0;
    //    else return result;
    //}
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    HANDLE g_hInputFile = NULL;

    int execCommand(char progname[], char option1[]) {
        SECURITY_ATTRIBUTES saAttr;

        // Set the bInheritHandle flag so pipe handles are inherited. 

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // Create a pipe for the child process's STDOUT. 

        if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
            return -1;

        // Ensure the read handle to the pipe for STDOUT is not inherited.

        if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
            return -1;

        // Create a pipe for the child process's STDIN. 

        if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
            return -1;

        // Ensure the write handle to the pipe for STDIN is not inherited. 

        if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
            return -1;

        // Create the child process. 

        CreateChildProcess(progname, option1);

        return 0;
    }


    void CreateChildProcess(char prog[], char option1[])
        // Create a child process that uses the previously created pipes for STDIN and STDOUT.
    {
        wchar_t* wtext = new wchar_t[strlen(option1) + 2];
        wtext[0] = L' '; // Add a space at the start of the cli arg.
        std::mbstowcs(&wtext[1], option1, strlen(option1) + 1);//Plus null
        LPWSTR szCmdline = wtext;

        wchar_t *wtextprog = new wchar_t[strlen(prog)+1];
        std::mbstowcs(wtextprog, prog, strlen(prog) + 1);//Plus null
        LPWSTR progname = wtextprog;

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        BOOL bSuccess = FALSE;

        // Set up members of the PROCESS_INFORMATION structure. 

        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        // Set up members of the STARTUPINFO structure. 
        // This structure specifies the STDIN and STDOUT handles for redirection.

        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = g_hChildStd_OUT_Wr;
        siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
        siStartInfo.hStdInput = g_hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // Create the child process. 

        bSuccess = CreateProcess(progname,
            szCmdline,     // command line 
            NULL,          // process security attributes 
            NULL,          // primary thread security attributes 
            TRUE,          // handles are inherited 
            0,             // creation flags 
            NULL,          // use parent's environment 
            NULL,          // use parent's current directory 
            &siStartInfo,  // STARTUPINFO pointer 
            &piProcInfo);  // receives PROCESS_INFORMATION 

         // If an error occurs, exit the application. 
        if (!bSuccess) {
            delete[] wtextprog;
            delete[] wtext;
            ErrorExit(TEXT("CreateProcess"));
            return;
        }
        else
        {   
            delete[] wtextprog;
            delete[] wtext;
            // Close handles to the child process and its primary thread.
            // Some applications might keep these handles to monitor the status
            // of the child process, for example. 

            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);

            // Close handles to the stdin and stdout pipes no longer needed by the child process.
            // If they are not explicitly closed, there is no way to recognize that the child process has ended.

            CloseHandle(g_hChildStd_OUT_Wr);
            CloseHandle(g_hChildStd_IN_Rd);
        }
    }


    void ErrorExit(const wchar_t* lpszFunction)

        // Format a readable error message, display a message box, 
        // and exit from the application.
    {
        LPVOID lpMsgBuf;
        LPVOID lpDisplayBuf;
        DWORD dw = GetLastError();

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);

        lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
            (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
        StringCchPrintf((LPTSTR)lpDisplayBuf,
            LocalSize(lpDisplayBuf) / sizeof(TCHAR),
            TEXT("%s failed with error %d: %s"),
            lpszFunction, dw, lpMsgBuf);
        MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);
        ExitProcess(1);
    }


    std::string execCtrlCmds(std::vector<std::string> cmdArgs)
    {
        std::string cmd = std::string(boost::algorithm::join(cmdArgs, " "));
        SECURITY_ATTRIBUTES saAttr;
        // Set the bInheritHandle flag so pipe handles are inherited. 

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // Create a pipe for the child process's STDOUT. 

        if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
            ErrorExit(TEXT("StdoutRd CreatePipe"));

        // Ensure the read handle to the pipe for STDOUT is not inherited.

        if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
            ErrorExit(TEXT("Stdout SetHandleInformation"));

        // Create a pipe for the child process's STDIN. 

        if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
            ErrorExit(TEXT("Stdin CreatePipe"));

        // Ensure the write handle to the pipe for STDIN is not inherited. 

        if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
            ErrorExit(TEXT("Stdin SetHandleInformation"));

        // Create the child process. 

        CreateCtrlChildProcess(cmd);

        // Get a handle to an input file for the parent. 
        // This example assumes a plain text file and uses string output to verify data flow. 

        // Read from pipe that is the standard output for child process. 
        auto result = ReadFromPipe();

        // The remaining open handles are cleaned up when this process terminates. 
        // To avoid resource leaks in a larger application, close handles explicitly. 

        return std::string(boost::algorithm::join(result, ""));
    }


    void CreateCtrlChildProcess(std::string cmd)
        // Create a child process that uses the previously created pipes for STDIN and STDOUT.
    {
        wchar_t* wtext = new wchar_t[cmd.size() + 1];
        std::mbstowcs(&wtext[0], cmd.c_str(), cmd.size() + 1);//Plus null
        LPWSTR szCmdline = wtext;

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        BOOL bSuccess = FALSE;

        // Set up members of the PROCESS_INFORMATION structure. 

        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        // Set up members of the STARTUPINFO structure. 
        // This structure specifies the STDIN and STDOUT handles for redirection.

        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = g_hChildStd_OUT_Wr;
        siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
        siStartInfo.hStdInput = g_hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // Create the child process. 

        bSuccess = CreateProcess(NULL,
            szCmdline,     // command line 
            NULL,          // process security attributes 
            NULL,          // primary thread security attributes 
            TRUE,          // handles are inherited 
            0,             // creation flags 
            NULL,          // use parent's environment 
            NULL,          // use parent's current directory 
            &siStartInfo,  // STARTUPINFO pointer 
            &piProcInfo);  // receives PROCESS_INFORMATION 

         // If an error occurs, exit the application. 
        if (!bSuccess) {
            delete[] wtext;
            ErrorExit(TEXT("CreateProcess"));
        }
        else
        {
            delete[] wtext;
            // Close handles to the child process and its primary thread.
            // Some applications might keep these handles to monitor the status
            // of the child process, for example. 

            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);

            // Close handles to the stdin and stdout pipes no longer needed by the child process.
            // If they are not explicitly closed, there is no way to recognize that the child process has ended.

            CloseHandle(g_hChildStd_OUT_Wr);
            CloseHandle(g_hChildStd_IN_Rd);
        }
    }


    std::vector<std::string> ReadFromPipe(void)
    {
        const int BUFSIZE = 4096;
        DWORD dwRead, dwWritten;
        CHAR chBuf[BUFSIZE];
        BOOL bSuccess = FALSE;
        HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

        std::vector<std::string> childProgOutput;

        for (;;)
        {
            bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE - 1, &dwRead, NULL);
            if (!bSuccess || dwRead == 0) break;

            chBuf[dwRead] = '\0';
            childProgOutput.push_back(std::string(chBuf));
        }
        return childProgOutput;
    }


    LSTATUS ReadRegistry(LPCWSTR sPath, LPCWSTR sKey, LPWSTR pBuffer, LPDWORD pBufferSize)
    {
        HKEY hKey;
        LSTATUS nResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, sPath,
            0, KEY_READ | KEY_WOW64_64KEY, &hKey);

        if (nResult == ERROR_SUCCESS)
        {
            nResult = ::RegQueryValueEx(hKey, sKey, NULL, NULL,
                (LPBYTE)pBuffer, pBufferSize);

            RegCloseKey(hKey);
        }

        return (nResult);
    }


    void sleep_t(unsigned int seconds){
        Sleep(seconds*1000);
    }
}
#endif