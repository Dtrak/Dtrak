#pragma once
#include "common.h"
#include "commonutils.h"
#include "pem.h"
#include "rsa.h"
#include "sha.h"
#include "base64.h"
#include "secblock.h"
#include "osrng.h"
#include "files.h"
#include "pssr.h"
#include "hex.h"
#include "base64.h"


namespace nativehost
{

    struct RegexMatchControl
    {
        std::string regex;
        std::string type;
        std::string expected_output;
        int regex_group = 0;
        bool result=0;
    };


    struct CtrlPlatformCommands
    {
        std::string name;
        std::string directive;
        std::string program_name;
        std::string file_to_read;
        std::string command;
        
        std::vector<RegexMatchControl> rgxctrl;

        unsigned long score=0;
        int result=0;
    };


    enum TrustLevel
    {
        TRUST_LOW = 0,
        TRUST_MEDIUM,
        TRUST_HIGH
    };


    enum RuqoCommMode
    {
        HEALTH_CHECK=0,
        READ_VAULT,
        WRITE_VAULT
    };


    class ControlPlatform{

        private:
            std::shared_ptr<std::vector<CtrlPlatformCommands*>> cmdStructList;
            unsigned int minscoreHigh=0;
            unsigned int minscoreMed=0;
            unsigned int totalScore=0;


        public:
            std::string configversion;
            TrustLevel trust;
            std::string machineID;
            TrustLevel getTrustLevel(void);
            TrustLevel getZeroTrustLevel(void);
            void regexOperations(const std::string& mode);
            void runFileReadOperations(void);
            void runSubCommandOperations(void);
            std::shared_ptr<std::vector<CtrlPlatformCommands*>> readControlPlatformFile(std::string interceptorPath);
            std::string getMachineID(void);
            unsigned int getTotalScore(void);
            std::shared_ptr<std::vector<CtrlPlatformCommands*>> getCommandStruct(void);

            ControlPlatform(std::string interceptorPath);
            ~ControlPlatform();

    };

    class NHUtils{
        public:
            static std::string executeRegex(const std::string &data, const std::string &regex, int group);
            static void sendMessageToExtension(std::string &jsonMessage);
            static std::string readExtensionMessage();
            static boost::property_tree::ptree parseExtensionMessage(std::string &message); 
            // static std::string getControlResultsJson(std::map<std::string, std::string> &commandResultsMap);
            static std::string getControlResultsJson(ControlPlatform &ctrlPlat);
            static std::string getErrorJson(std::string e);
            static int compVersions(const std::string &ver1, const std::string &ver2);
            static std::string readFileToString(const std::string& filename);
            static bool compBits(unsigned int cmdResult, unsigned int mask);

            // Utils for password vault
            static std::string urlToJson(const std::string &url, RuqoCommMode mode);
            static std::string credentialsToJson(const std::string &url, const std::string &username, const std::string &password, RuqoCommMode mode);
    };

    void execControlCommands(std::string interceptorPath);
}

namespace nativehost
{
    namespace debug
    {
        void debugCtrlStruct(std::shared_ptr<std::vector<nativehost::CtrlPlatformCommands*>> data);
    }
}

#ifdef IN_DEBUG_BUILD
    #define DEBUG_CTRL_DATA(D) nativehost::debug::debugCtrlStruct(D)
    #define DEBUG_PRINT_NH(E) std::cout << "\n[DEBUG]: " << E << std::endl
#endif
#ifdef IN_DIST_BUILD
    #define DEBUG_CTRL_DATA(D)
    #define DEBUG_PRINT_NH(E)
#endif