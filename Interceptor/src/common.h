#pragma once

#include <fstream>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <cstdio>
#include <map>
#include <sstream>
#include <vector>
#include <regex>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <bitset>
#include <boost/algorithm/string/join.hpp>


#define PIPE_READ 0
#define PIPE_WRITE 1

#ifdef IN_PLATFORM_WINDOWS
    #include "win\coreWin.h"
    const char PLATFORM[] = "WIN64";
#endif

#ifdef IN_PLATFORM_POSIX
    #include "posix/corePosix.h"
    #ifdef IN_OS_LINUX
        const char PLATFORM[] = "LINUX";
    #endif
    #ifdef IN_OS_MAC
        const char PLATFORM[] = "MAC";
    #endif
#endif