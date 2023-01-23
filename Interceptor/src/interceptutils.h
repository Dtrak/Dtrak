#pragma once

#include "common.h"
#include "commonutils.h"
#include "nativehost.h"

namespace transporter
{
    std::string GetInput();
    void OutputPipeTogit(std::string& data, int filedesc, bool silent);

    class Commit{
        public:
            static std::string generateDeeplink(std::string deepLinkData);
            static void verifyCommit(int filedesc, std::string tempfile, std::string binPath, bool silent);
            static void signCommit(int filedesc, std::string KeyID, std::string binPath, bool silent);
            static std::string getSignCommitJson(std::string &data, std::string &keyID);
            static std::string getVerifyCommitJson(std::string &data, std::string &tempfilepath);
    };
}

namespace utils
{
    enum Mode
    {
        MODE_COMMIT_SIGN = 0,
        MODE_COMMIT_VERIFY,
        MODE_RUQO_COMM,
        MODE_FILE_ENCRYPT,
        MODE_FILE_DECRYPT,
        MODE_UNDEFINED
    };

    enum FileDescriptor
    {
        C_STDOUT = 1,
        C_STDERR,
        C_FD_UNDEFINED
    };

    struct ProcessData
    {
        Mode mode = MODE_UNDEFINED;
        FileDescriptor fd = C_FD_UNDEFINED;
        std::string keyID;
        std::string filepath;
        std::string interceptorpath;
    };

    std::map<std::string, std::string>* parseArgs(int argc, char** argv);
    std::shared_ptr<ProcessData> getProcessModeData(std::map<std::string, std::string>* parsedargs);
    std::string base64_encode(const std::string &s);
    void safeCopy(char* dst, const char* src, size_t s_dst);

    void execAsPerMode(std::shared_ptr<ProcessData> modeData);
}

namespace utils
{
    namespace debug
    {
        void debugParsedArgs(std::map<std::string, std::string>* data);
        void debugModeData(std::shared_ptr<utils::ProcessData> data);
    }
}

#ifdef IN_DEBUG_BUILD
    #define DEBUG_MODE_DATA(A) utils::debug::debugModeData(A)
    #define DEBUG_PARSED_ARGS(B) utils::debug::debugParsedArgs(B)
    #define DEBUG_PRINT(C) std::cout << "\n[DEBUG]: " << C << std::endl
#endif
#ifdef IN_DIST_BUILD
    #define DEBUG_MODE_DATA(A)
    #define DEBUG_PARSED_ARGS(B)
    #define DEBUG_PRINT(C)
#endif