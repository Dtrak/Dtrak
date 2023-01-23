#pragma once
#include "common.h"

namespace cutils{
    const std::string LOCAL_IP = "127.0.0.1";
    const std::string PORT = "8124";
    const int SERVER_PORT = 8125;
    const unsigned int SECONDS_TO_WAIT = 2;
    
    std::vector<std::string> split(const std::string &str, char delim, int limit);
    void safeCopy(char* dst, const char* src, size_t s_dst);
    int callVr5(std::string authverifyPath, std::string mode);
    std::string readConfig(std::string interceptorPath);
    std::string SocketSendData(std::string& data, const std::string& ip, const std::string& port);
    std::string SocketReceiveData(const int port);
}