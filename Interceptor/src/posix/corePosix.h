#pragma once
#ifdef IN_PLATFORM_POSIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <boost/algorithm/string/join.hpp>

namespace platform
{
    void forkedProgram(char* progname, char* option1);
    int execCommand(char* progname, char* option1);
    void sleep_t(unsigned int seconds);
    std::string execCtrlCmds(std::vector<std::string> cmdArgs);

    #ifdef IN_OS_MAC
    #include <IOKit/IOKitLib.h>
    namespace macos{        
        void get_platform_uuid(char * buf, int bufSize);
        std::string get_uuid(void);
    }
    #endif
}

#endif