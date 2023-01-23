#include "interceptutils.h"

namespace transporter
{

    std::string GetInput() {
        std::string s;
        std::getline(std::cin, s, static_cast<char>(EOF));
        return s;
    }


    void OutputPipeTogit(std::string& data, int filedesc, bool silent) {
        std::string tp;
        std::istringstream iss(data);
        while (getline(iss, tp)) { //read data from file object and put it into string.
            if (tp.rfind("[GNUPG:] ", 0) == 0 || tp.rfind("[AUTHVERIFY:] ", 0) == 0) {
                if (!silent) {
                    if (filedesc == 2) {
                        std::cerr << tp << std::endl;
                    }
                    else {
                        std::cerr << tp << std::endl;
                        std::cout << tp << std::endl;
                    }
                }
            }
            else {
                std::cout << tp << std::endl; //print the data of the string
                // Get the trustlevel to zero
            }
        }
    }


    std::string Commit::generateDeeplink(std::string deepLinkData){
        std::string deeplink_b64 = utils::base64_encode(deepLinkData);
        std::string deeplink = "pureid://git/" + deeplink_b64;
        return deeplink;
    }

    std::string Commit::getSignCommitJson(std::string &data, std::string &keyID){
        namespace pt = boost::property_tree;
        pt::ptree root;
        std::ostringstream oss;

        root.put("mode", "sign");
        root.put("message", data);
        root.put("keyid", keyID);

        pt::write_json(oss, root);
        return oss.str();
    }


    std::string Commit::getVerifyCommitJson(std::string &data, std::string &tempfilepath){
        namespace pt = boost::property_tree;
        pt::ptree root;
        std::ostringstream oss;

        root.put("mode", "verify");
        root.put("message", data);
        root.put("signature_file", tempfilepath);

        pt::write_json(oss, root);
        return oss.str();
    }


    void Commit::verifyCommit(int filedesc, std::string tempfile, std::string binPath, bool silent){
        std::string gitData = transporter::GetInput();
        std::string dataWithMode = getVerifyCommitJson(gitData, tempfile);
        std::string ack = cutils::SocketSendData(dataWithMode, cutils::LOCAL_IP, cutils::PORT);
        DEBUG_PRINT(ack);
        if (ack != "ACK"){
            std::string authverifyPath = cutils::readConfig(binPath);
            cutils::callVr5(authverifyPath, "pureid://git/");
            // Wait for vr5 to launch
            for (int i=0; i<5; i++){
                DEBUG_PRINT("Waiting for VR5 to start...");
                platform::sleep_t(cutils::SECONDS_TO_WAIT);
                ack = cutils::SocketSendData(dataWithMode, cutils::LOCAL_IP, cutils::PORT);
                if (ack == "ACK"){
                    break;
                } else{
                    if (i==2){
                        std::string error = "Could Not start VR5!";
                        OutputPipeTogit(error, filedesc, silent);
                        return;
                    }
                }
            }
        }
        DEBUG_PRINT("Server started");
        std::string response = cutils::SocketReceiveData(cutils::SERVER_PORT);
        DEBUG_PRINT(response);
        OutputPipeTogit(response, filedesc, silent);
    }


    void Commit::signCommit(int filedesc, std::string KeyID, std::string binPath, bool silent){
        std::string gitData = transporter::GetInput();
        std::string dataWithMode = getSignCommitJson(gitData, KeyID);
        std::string ack = cutils::SocketSendData(dataWithMode, cutils::LOCAL_IP, cutils::PORT);
        DEBUG_PRINT(ack);
        if (ack != "ACK"){
            std::string authverifyPath = cutils::readConfig(binPath);
            cutils::callVr5(authverifyPath, "pureid://git/");
            // Wait for vr5 to launch
            for (int i=0; i<4; i++){
                DEBUG_PRINT("Waiting for VR5 to start...");
                platform::sleep_t(cutils::SECONDS_TO_WAIT);
                ack = cutils::SocketSendData(dataWithMode, cutils::LOCAL_IP, cutils::PORT);
                if (ack == "ACK"){
                    break;
                } else{
                    if (i==2){
                        std::string error = "Could Not start VR5!";
                        OutputPipeTogit(error, filedesc, silent);
                        return;
                    }
                }
            }
        }
        DEBUG_PRINT("Server started");
        std::string response = cutils::SocketReceiveData(cutils::SERVER_PORT);
        DEBUG_PRINT(response);
        OutputPipeTogit(response, filedesc, silent);
    }
}

namespace utils
{   
    std::map<std::string, std::string>* parseArgs(int argc, char** argv){
        // Map the CLI args to their values.
        // args such as --status-fd=2 are split using the '=' sign.
        // args such as --verify file.tmp are processed as key,value.
        // args without '-' at the start are treated as keys without value.
        auto parsedMap = new std::map<std::string, std::string>();
        bool resultFlag = false;
        std::string temp;
        // Add interceptor path
        if (argc > 0)
        parsedMap->insert(std::pair<std::string, std::string>("-interceptor-path", argv[0]));
        for (int i=1; i<argc; i++){
            if (resultFlag){
                resultFlag = false;
                continue;
            }
            // Split at '='
            auto splitRes = cutils::split(argv[i], '=', 1);
            if (splitRes.size() == 2){
                parsedMap->insert(std::pair<std::string, std::string>(splitRes[0], splitRes[1]));
                continue;
            }
            // Split at ':'
            auto splitRes_b = cutils::split(argv[i], ':', 1);
            if (splitRes_b.size() == 2){
                parsedMap->insert(std::pair<std::string, std::string>(splitRes_b[0], splitRes_b[1]));
                continue;
            }
            temp.assign(argv[i]);
            if(temp.rfind("-", 0)==0 && i!=argc-1){
                std::string nextArg = std::string(argv[i+1]);
                if (nextArg.rfind("-", 0)==0){
                    parsedMap->insert(std::pair<std::string, std::string>(temp, ""));
                }
                else{
                    parsedMap->insert(std::pair<std::string, std::string>(temp, nextArg));
                    resultFlag = true;
                }
            }
            else {
                parsedMap->insert(std::pair<std::string, std::string>(temp, ""));
            }

        }
        return parsedMap;
    }


    std::shared_ptr<ProcessData> getProcessModeData(std::map<std::string, std::string>* parsedargs){
        // Returns the operation mode and the data required for that mode.
        // Uses a shared pointer to manage the struct lifetime.
        std::shared_ptr<ProcessData> modeData = std::make_shared<ProcessData>();
        try{
            if (!(parsedargs->find("-interceptor-path") == parsedargs->end())){
                modeData->interceptorpath = parsedargs->at("-interceptor-path");
            }
            if (!(parsedargs->find("--status-fd") == parsedargs->end())){
                modeData->fd = std::stoi(parsedargs->at("--status-fd")) > 1 ? C_STDERR : C_STDOUT;
            }
            if (!(parsedargs->find("-bsau") == parsedargs->end())){
                modeData->mode = MODE_COMMIT_SIGN;
                modeData->keyID = parsedargs->at("-bsau");
            }
            else if (!(parsedargs->find("--verify") == parsedargs->end())){
                modeData->mode = MODE_COMMIT_VERIFY;
                modeData->filepath = parsedargs->at("--verify");
            }
            else if (!(parsedargs->find("chrome-extension") == parsedargs->end())){
                modeData->mode = MODE_RUQO_COMM;
                modeData->fd = C_STDOUT;
            }
        } 
        catch (std::invalid_argument e){
            std::cout << "Invalid Argument" << std::endl;
        }
        return modeData;
    }


    std::string base64_encode(const std::string &s){
        static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        size_t i=0,ix=0,leng = s.length();
        std::stringstream q;
    
        for(i=0,ix=leng - leng%3; i<ix; i+=3)
        {
            q<< base64_chars[ (s[i] & 0xfc) >> 2 ];
            q<< base64_chars[ ((s[i] & 0x03) << 4) + ((s[i+1] & 0xf0) >> 4)  ];
            q<< base64_chars[ ((s[i+1] & 0x0f) << 2) + ((s[i+2] & 0xc0) >> 6)  ];
            q<< base64_chars[ s[i+2] & 0x3f ];
        }
        if (ix<leng)
        {
            q<< base64_chars[ (s[ix] & 0xfc) >> 2 ];
            q<< base64_chars[ ((s[ix] & 0x03) << 4) + (ix+1<leng ? (s[ix+1] & 0xf0) >> 4 : 0)];
            q<< (ix+1<leng ? base64_chars[ ((s[ix+1] & 0x0f) << 2) ] : '=');
            q<< '=';
        }
        return q.str();
    }


    void safeCopy(char* dst, const char* src, size_t s_dst){
        size_t s_src = strlen(src);
        size_t s_data_to_cpy;

        *dst = '\0';

        if ( s_dst < s_src ){
            s_data_to_cpy = s_dst - 1;

        } else {
            s_data_to_cpy = s_src;
        }
        strncat(dst, src, s_data_to_cpy);
    }


    void execAsPerMode(std::shared_ptr<ProcessData> modeData){
        switch (modeData->mode)
        {
            case MODE_COMMIT_SIGN:
            {   
                bool silent = false;

                if (strcmp(PLATFORM, "WIN64") == 0){
                    silent = true;
                }
                transporter::Commit::signCommit(
                    modeData->fd, modeData->keyID, modeData->interceptorpath, silent
                );
                break;
            }
            case MODE_COMMIT_VERIFY:
            {
                bool silent = false;

                transporter::Commit::verifyCommit(
                    modeData->fd, modeData->filepath, modeData->interceptorpath, silent
                );
                break;
            }
            case MODE_RUQO_COMM:
            {
                nativehost::execControlCommands(modeData->interceptorpath);
                break;
            }
            
            default:
                break;
        }
    }
}

namespace utils
{
    namespace debug
    {
        void debugModeData(std::shared_ptr<utils::ProcessData> data){
            std::cout << "=============================Mode Struct============================" << std::endl;
            std::cout << "File Descriptor  : " << data->fd << std::endl;
            std::cout << "Key ID           : " << data->keyID << std::endl;
            std::cout << "Mode             : " << data->mode << std::endl;
            std::cout << "File Path        : " << data->filepath << std::endl;
            std::cout << "Interceptor Path : " << data->interceptorpath << std::endl;
            std::cout << "=============================Mode Struct============================\n" << std::endl;
        }

        void debugParsedArgs(std::map<std::string, std::string>* data){
            std::cout << "=============================Args============================" << std::endl;
            for (auto it=data->begin(); it!=data->end(); it++){
                std::cout << it->first << "\t---->>\t" << it->second << std::endl;
            }
            std::cout << "=============================Args============================\n" << std::endl;
        }
    }
}