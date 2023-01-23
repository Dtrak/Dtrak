#include "nativehost.h"
#include <fstream>

namespace nativehost
{
    namespace pt = boost::property_tree;
    int sigVerify();
// ===================================Class Definitions=================================================

    ControlPlatform::ControlPlatform(std::string interceptorPath){
        this->cmdStructList = readControlPlatformFile(interceptorPath);
        this->trust = getTrustLevel();
        this->machineID = getMachineID();
    }

    ControlPlatform::~ControlPlatform(){
        // Deallocate the structures and clear vector
        for (auto it=cmdStructList->begin(); it!=cmdStructList->end(); it++){
            delete (*it);
        }
        cmdStructList->clear();
    }


    unsigned int ControlPlatform::getTotalScore(void){
        return this->totalScore;
    };


    std::shared_ptr<std::vector<CtrlPlatformCommands*>> ControlPlatform::readControlPlatformFile(
        std::string interceptorPath
    ){

        auto ctrlCmds = std::make_shared<std::vector<CtrlPlatformCommands*>>();

        std::filesystem::path configfilepath(interceptorPath);
        configfilepath = configfilepath.parent_path();
        configfilepath /= "interceptor_control.config.json";

        std::fstream configfile;
        configfile.open(configfilepath, std::ios::in);

        if (!configfile) throw std::runtime_error(std::string("Configuration file not found!"));
        std::string content;
        content.assign((std::istreambuf_iterator<char>(configfile)),
                        (std::istreambuf_iterator<char>()));
        configfile.close();
        pt::ptree jsontree;
        std::stringstream ss; ss << content;
        pt::read_json(ss, jsontree);
        this->minscoreHigh = jsontree.get<int>("minscore_high");
        this->minscoreMed = jsontree.get<int>("minscore_med");
        this->configversion = jsontree.get<std::string>("version");
        for(pt::ptree::value_type &kv : jsontree.get_child("controls")){
            // CRITICAL: Delete the memory manually BEFORE calling vector::clear.
            auto cmdstruct = new CtrlPlatformCommands();
            cmdstruct->name = kv.second.get<std::string>("name");
            cmdstruct->directive = kv.second.get<std::string>("directive");
            cmdstruct->program_name = kv.second.get<std::string>("program_name");
            cmdstruct->file_to_read = kv.second.get<std::string>("file_to_read");
            cmdstruct->command = kv.second.get<std::string>("command");
            cmdstruct->score = kv.second.get<unsigned int>("score");

            for (pt::ptree::value_type& kv2 : kv.second.get_child("regex_control")) {
                RegexMatchControl regexstruct;
                regexstruct.regex = kv2.second.get<std::string>("regex");
                regexstruct.regex_group = kv2.second.get<int>("regex_group");
                regexstruct.type = kv2.second.get<std::string>("type");
                regexstruct.expected_output = kv2.second.get<std::string>("expected_output");
                cmdstruct->rgxctrl.push_back(regexstruct);
            }

            ctrlCmds->push_back(cmdstruct);
        }
        return ctrlCmds;
    }


    void ControlPlatform::regexOperations(const std::string& mode) {
        int comparisonresult = 0;
        std::string regexed;
        int _totalScore = 0;
        std::string Data;

        for (auto it = this->cmdStructList->begin(); it != this->cmdStructList->end(); it++) {
            _totalScore = 0;
            if ((*it)->directive != mode) continue;
            if ((*it)->directive == "run_prog") {
                auto split_args = cutils::split((*it)->command, ' ', 100);
                Data = platform::execCtrlCmds(split_args);
            }
            else {
                Data = NHUtils::readFileToString((*it)->file_to_read);
            }

            for (auto& rgx : (*it)->rgxctrl) {
                if (rgx.regex == "") rgx.regex = ".*";
                regexed = NHUtils::executeRegex(Data, rgx.regex, rgx.regex_group);
                if (rgx.type == "version") {
                    comparisonresult = NHUtils::compVersions(regexed, rgx.expected_output);
                    rgx.result = comparisonresult >= 0 ? true : false;
                }
                if (rgx.type == "match") {
                    rgx.result = (regexed == rgx.expected_output);
                }
                if (rgx.type == "binary") {
                    rgx.result = NHUtils::compBits(std::stoi(regexed), std::stoi(rgx.expected_output));
                }
                _totalScore += rgx.result == true ? 1 : 0;

            }
            (*it)->result = (_totalScore == (*it)->rgxctrl.size());
        }
    }


    void ControlPlatform::runFileReadOperations(){
        this->regexOperations("read_file");
    }


    void ControlPlatform::runSubCommandOperations(){
        this->regexOperations("run_prog");
    }


    TrustLevel ControlPlatform::getTrustLevel(){
        this->totalScore = 0;
        for (auto it=cmdStructList->begin(); it!=cmdStructList->end(); it++){
            this->totalScore += (*it)->result * (*it)->score;
        }
        DEBUG_CTRL_DATA(this->cmdStructList);
        DEBUG_PRINT_NH(std::bitset<sizeof(unsigned int)*8>(totalScore));
        if (totalScore <= this->minscoreMed) return TRUST_LOW;
        if (totalScore >= this->minscoreHigh) return TRUST_HIGH;
        return TRUST_MEDIUM;
    }


        TrustLevel ControlPlatform::getZeroTrustLevel(){
        this->totalScore = 0;
        return TRUST_LOW;
    }


    std::string ControlPlatform::getMachineID(void){
        #ifdef IN_OS_LINUX
            std::fstream configfile;
            configfile.open("/var/lib/dbus/machine-id", std::ios::in);

            if (!configfile) return "";
            std::string content;
            getline(configfile, content);
            configfile.close();
            return content;
        #elif IN_PLATFORM_WINDOWS
            const int BUFFER_SIZE = 2048;

            WCHAR sBuffer[BUFFER_SIZE]; // 2048 bytes
            DWORD nBufferSize = BUFFER_SIZE * sizeof(WCHAR);

            ZeroMemory(sBuffer, nBufferSize);
            LSTATUS nResult = platform::ReadRegistry(L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid",
                sBuffer, &nBufferSize);
            if (nResult != ERROR_SUCCESS) return "";
            std::wstring ws(sBuffer);
            std::string regVal(ws.begin(), ws.end());
            return regVal;
        #elif IN_OS_MAC
            return platform::macos::get_uuid();
        #endif
    }


    std::shared_ptr<std::vector<CtrlPlatformCommands*>> ControlPlatform::getCommandStruct(void){
        return this->cmdStructList;
    }


// ==========================================Class END==========================================================



    int NHUtils::compVersions(const std::string &ver1, const std::string &ver2) {
        // If ver1 > ver2, return 1;
        // If ver2 > ver1, return -1;
        // If ver1 == ver2, return 0;
        unsigned major1 = 0, minor1 = 0, bugfix1 = 0, alpha1 = 0;
        unsigned major2 = 0, minor2 = 0, bugfix2 = 0, alpha2 = 0;
        std::sscanf(ver1.c_str(), "%u.%u.%u.%u", &major1, &minor1, &bugfix1, &alpha1);
        std::sscanf(ver2.c_str(), "%u.%u.%u.%u", &major2, &minor2, &bugfix2, &alpha2);
        if (major1 < major2) return -1;
        if (major1 > major2) return 1;
        if (minor1 < minor2) return -1;
        if (minor1 > minor2) return 1;
        if (bugfix1 < bugfix2) return -1;
        if (bugfix1 > bugfix2) return 1;
        if (alpha1 < alpha2) return -1;
        if (alpha1 > alpha2) return 1;
        return 0;
    }


    bool NHUtils::compBits(unsigned int cmdResult, unsigned int mask) {
        return ((cmdResult & mask) == mask);
    }


    std::string NHUtils::executeRegex(const std::string &data, const std::string &regex, int group){
        std::regex r(regex);
        std::smatch m;
        std::regex_search(data, m, r);
        if (m.size() > group)
            return m[group];
        return std::string("");
    }


    std::string NHUtils::readFileToString(const std::string& filename){
        std::string fileData = "";
        std::fstream fileh;
        fileh.open(filename, std::ios::in);

        if (!fileh) return fileData;
        fileData.assign((std::istreambuf_iterator<char>(fileh)),
                        (std::istreambuf_iterator<char>()));
        fileh.close();
        return fileData;       
    }


    void NHUtils::sendMessageToExtension(std::string &jsonMessage){

        unsigned int len = jsonMessage.length();
        std::cout << char(len>>0)
                << char(len>>8)
                << char(len>>16)
                << char(len>>24);
            
        std::cout << jsonMessage << std::flush;
        std::ofstream ofs("test.txt", std::ofstream::trunc);

        ofs << "lorem ipsum";

        ofs.close();

    }


    std::string NHUtils::readExtensionMessage(){
        unsigned int length = 0;
        for (int i = 0; i < 4; i++)
        {
            unsigned int read_char = getchar();
            length = length | (read_char << i*8);
        }

        //read the json-message
        std::string msg = "";
        for (int i = 0; i < length; i++)
        {
            msg += getchar();
        }
        return msg;
    }


    pt::ptree NHUtils::parseExtensionMessage(std::string &message){
        pt::ptree extensionJson;
        std::stringstream ss;
        ss << message;
        pt::read_json(ss, extensionJson);
        return extensionJson;
    }

    std::string NHUtils::getControlResultsJson(ControlPlatform &ctrlPlat){
        pt::ptree root;
        std::ostringstream oss;

        root.put("control.version", ctrlPlat.configversion);
        root.put("control.TrustLevel", std::to_string(ctrlPlat.trust));
        root.put("control.MachineId", ctrlPlat.machineID);
        root.put("control.TotalScore", std::to_string(ctrlPlat.getTotalScore()));

        pt::ptree children;

        auto ctrlStruct = ctrlPlat.getCommandStruct();
        for (auto it=ctrlStruct->begin(); it!=ctrlStruct->end(); it++){
            pt::ptree temp;
            temp.put("Name", (*it)->name);
            temp.put("Result", (*it)->result);
            children.push_back(std::make_pair("", temp));
        }
        root.add_child("commands", children);

        pt::write_json(oss, root);
        return oss.str();
    }


    std::string NHUtils::getErrorJson(std::string e){
        pt::ptree root;
        std::ostringstream oss;

        root.put("status", "fail");
        root.put("reason", e);

        pt::write_json(oss, root);
        return oss.str();
    }


    // Utils for password vault
    std::string NHUtils::urlToJson(const std::string &url, RuqoCommMode mode){
        namespace pt = boost::property_tree;
        pt::ptree root;
        std::ostringstream oss;

        root.put("mode", mode);
        root.put("url", url);

        pt::write_json(oss, root);
        return oss.str();
    }


    std::string NHUtils::credentialsToJson(
        const std::string &url, const std::string &username, const std::string &password, RuqoCommMode mode
    ){
        namespace pt = boost::property_tree;
        pt::ptree root;
        std::ostringstream oss;

        root.put("mode", mode);
        root.put("url", url);
        root.put("username", username);
        root.put("password", password);

        pt::write_json(oss, root);
        return oss.str();
    }

// add function here
    void execControlCommands(std::string interceptorPath){
        int sig = sigVerify();
        if(sig)
        {
            std::string messageToExt;
            std::string url, username, password;

            // Get Data from RUQO
            // std::string messageFromExt = NHUtils::readExtensionMessage();
            // auto jsontree = NHUtils::parseExtensionMessage(messageFromExt);
            // int mode = jsontree.get<int>("mode");
            int mode = 0;

            switch (mode)
            {
                case HEALTH_CHECK:
                    try{

                        ControlPlatform ctrlPlat(interceptorPath);
                        ctrlPlat.runFileReadOperations();
                        ctrlPlat.runSubCommandOperations();
                        ctrlPlat.trust = ctrlPlat.getTrustLevel();
                        messageToExt = NHUtils::getControlResultsJson(ctrlPlat);
                        std::cout<< messageToExt << std::endl;
                        std::ofstream ofs("test.txt", std::ofstream::trunc);
                        ofs <<  messageToExt << std::flush;
                        ofs.close();
                        
                    } catch (boost::wrapexcept<boost::property_tree::ptree_bad_path> &e){
                        messageToExt = NHUtils::getErrorJson("Missing node in config file!");
                    } catch (std::runtime_error &e){
                        messageToExt = NHUtils::getErrorJson(e.what());
                    }
                    // DEBUG_PRINT_NH(messageToExt);
                    // messageToExt.erase(std::remove(messageToExt.begin(), messageToExt.end(), '\n'), messageToExt.end());
                    // NHUtils::sendMessageToExtension(messageToExt);


                
                default:
                    break;
            }
        }
        else
        {
            std::string messageToExt;
            ControlPlatform ctrlPlat(interceptorPath);
            ctrlPlat.runFileReadOperations();
            ctrlPlat.runSubCommandOperations();
            ctrlPlat.trust = ctrlPlat.getZeroTrustLevel();
            messageToExt = NHUtils::getErrorJson("Config file has been tampered with!!!");
            messageToExt = NHUtils::getControlResultsJson(ctrlPlat);
            std::cout<< messageToExt << std::endl;
            std::ofstream ofs("test.txt", std::ofstream::trunc);
            ofs <<  messageToExt << std::flush;
            ofs.close();
        }
    }
       
    // add new function


    CryptoPP::RSA::PublicKey loadPublicKey() {
        const std::string publickey_str =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAz4cetS8lMhYsx/YstOmZ\n"
        "a49us75YWzd6dy/8/NdQlb/fK+WG64Od16Y5BNlKg0fGBcCT6tX/FxoM9qIHnnda\n"
        "C4bbVwoCwR8QE1zV/c1I8RrHbgA/y+/HNY56oS0i7Ye12Ip42PJGGRLyYTHBXpaL\n"
        "adqqlhU09x91xY1jt258sKt6S+o1y8g9rMEB8wmUF/c9vWeCW25I4HHY7Z7tyAUz\n"
        "Fz6pUrHFtlqZ1apAqvD+k3Iu33LGIeHqQbJMUEfiahi/lLU4cuOrJskdY4597jHT\n"
        "EHbwyXbMCxsmB52FLduSe0BzVtYGvtmCWjY8HT/9h5O5WHcfd+N3OoaKtV58qX39\n"
        "YQIDAQAB\n"
        "-----END PUBLIC KEY-----\n";

        CryptoPP::RSA::PublicKey publicKey;

        try
        {
            CryptoPP::StringSource source(publickey_str, true);
            PEM_Load(source, publicKey);
        }
        catch(const CryptoPP::Exception& ex)
        {
            std::cerr << ex.what() << std::endl;
        }
        return publicKey;
    }

    CryptoPP::RSA::PrivateKey loadPrivateKey () {
        CryptoPP::RSA::PrivateKey privatekey;

        try
        {
            CryptoPP::FileSource source("keypair.pem", true);
            PEM_Load(source, privatekey);
        }
        catch(const CryptoPP::Exception& ex)
        {
            std::cerr << ex.what() << std::endl;
        }
        return privatekey;
    }


    int sigVerify()
    {
        // Generate key material

        CryptoPP::AutoSeededRandomPool prng;
        CryptoPP::RSA::PublicKey publicKey = loadPublicKey();

        std::string signature, sign;
    
        CryptoPP::FileSource fs2("/home/srishti/Desktop/neo/bin/Debug-linux-x86_64/outfile.sig", true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(signature)
            ) // Base64Decoder
        ); // StringSource
        // Create a verifier
        CryptoPP::byte result = 0;
        CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Verifier verifier(publicKey);
        CryptoPP::SignatureVerificationFilter filter(verifier, new CryptoPP::ArraySink(&result, sizeof(result)));

        // Wrap the data in sources
        CryptoPP::StringSource ss(signature, true);
        CryptoPP::FileSource fs("/home/srishti/Desktop/neo/bin/Debug-linux-x86_64/interceptor_control.config.json", true);

        // Add the data to the filter
        ss.TransferTo(filter);
        fs.TransferTo(filter);

        // Signal end of data. The filter will
        // verify the signature on the file
        filter.MessageEnd();

        if (result)
            return 1;
        else
            return 0;
    }

}


namespace nativehost
{
    namespace debug
    {
        void debugCtrlStruct(std::shared_ptr<std::vector<nativehost::CtrlPlatformCommands*>> data){
            std::cout << "=============================Ctrl============================" << std::endl;
            for (auto it=data->begin(); it!=data->end(); it++){
                std::cout << "\n";
                std::cout << "Name           : " << (*it)->name << "\n";
                std::cout << "Directive      : " << (*it)->directive << "\n";
                std::cout << "Program Name   : " << (*it)->program_name << "\n";
                std::cout << "File           : " << (*it)->file_to_read << "\n";
                std::cout << "Command        : " << (*it)->command << "\n";
                std::cout << "Score          : " << (*it)->score << "\n";
                for (auto rgx : (*it)->rgxctrl) {
                    std::cout << "\n";
                    std::cout << "    Regex             : " << rgx.regex << "\n";
                    std::cout << "    Regex Group       : " << rgx.regex_group << "\n";
                    std::cout << "    Type              : " << rgx.type << "\n";
                    std::cout << "    Expected Output   : " << rgx.expected_output << "\n";
                    std::cout << "    Result            : " << rgx.result << "\n";
                    std::cout << "\n";
                }
                std::cout << "Result         : " << (*it)->result << "\n";
                std::cout << std::endl;
            }
            std::cout << "=============================Ctrl============================" << std::endl;
        }
    }
}