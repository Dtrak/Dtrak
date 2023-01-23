#include "commonutils.h"

namespace cutils{
    using boost::asio::ip::tcp;

    std::vector<std::string> split(const std::string &str, char delim, int limit){
        // Split the command line args using the delimiter,
        // Works similar to python str.split, but limit does not
        // stop separating the string, just returns the first n + 1 elements. 
        std::vector<std::string> result;
        std::istringstream f(str);
        std::string s;
        int i=0;
        while (getline(f, s, delim) && i < limit+1) {
            result.push_back(s);
            i++;
        }
        return result;
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

    int callVr5(std::string authverifyPath, std::string mode) {
        int execresult;
        const int ARRAYSIZE = 150;
        char option1[ARRAYSIZE];
        char* prog = authverifyPath.data();
        cutils::safeCopy(option1, mode.c_str(), ARRAYSIZE);

        execresult = platform::execCommand(
            prog, option1
        );
        if (execresult != 0) {
            std::cerr << "Could not execute AUTHVERIFY! : " << execresult << std::flush;
            return -1;
        }
        return execresult;
    }

    std::string SocketSendData(std::string& data, const std::string& ip, const std::string& port) {
        try {
            boost::asio::io_context io_context;
            

            tcp::socket s(io_context);
            tcp::resolver resolver(io_context);
            boost::asio::connect(s, resolver.resolve(ip, port));

            boost::asio::write(s, boost::asio::buffer(data.c_str(), data.size()));


            boost::asio::streambuf b;
            boost::asio::read_until(s, b, '\0');
            std::istream is(&b);
            std::string line;
            std::getline(is, line, '\0');

            return line;
        }
        catch (std::exception& e)
        {
            return std::string("");
        }
    }


    std::string SocketReceiveData(const int port){
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::acceptor acceptor(
            ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port)
        );
        std::string line;
        for(;;){
            boost::asio::ip::tcp::socket sock(ioc);
            acceptor.accept(sock);

            try{
                boost::asio::streambuf b;
                boost::asio::read_until(sock, b, '\0');
                std::istream is(&b);
                std::getline(is, line, '\0');
                boost::asio::write(sock, boost::asio::buffer(line.c_str(), line.size()));
                break;

            } 
            catch (std::exception& e)
            {
                return std::string("");
            }
            sock.close();
        }
        return line;
    }


    std::string readConfig(std::string interceptorPath){
        using boost::property_tree::ptree;

        std::filesystem::path configfilepath(interceptorPath);
        configfilepath = configfilepath.parent_path();
        configfilepath /= "interceptor.config.json";

        std::fstream configfile;
        configfile.open(configfilepath, std::ios::in);
        if (configfile){
            std::string content;
            content.assign((std::istreambuf_iterator<char>(configfile)),
                            (std::istreambuf_iterator<char>()));
            configfile.close();
            ptree pt;
            std::stringstream ss; ss << content;
            read_json(ss, pt);
            return pt.get<std::string>("authverify");
        }
        return std::string("");
    }

}