#include "../common.h"
#include "corePosix.h"

#ifdef IN_PLATFORM_POSIX

namespace platform
{
    void forkedProgram(char* progname, char* option1){
        // char prog_name[] = "/usr/bin/xdg-open";
        char *const parmList[] = {progname, option1, NULL};
        execv(progname, parmList);
        exit(127);
    }

    int execCommand(char* progname, char* option1){
        pid_t pid = fork();
        if (pid == 0){
            // Child process
            forkedProgram(progname, option1);
        }
        int status=0;
        // waitpid(pid, &status, 0);
        return status;
    }

    void sleep_t(unsigned int seconds){
        sleep(seconds);
    }

    std::string execCtrlCmds(std::vector<std::string> cmdArgs){
        int status;
        int fd[2];
        if (pipe(fd) != 0){
            return "";
        }
        pid_t pid = fork();
        if (pid == 0){
            // Child process
            close(fd[PIPE_READ]);
            if (dup2(fd[PIPE_WRITE], STDOUT_FILENO) == -1) exit(128);
            close(fd[PIPE_WRITE]);

            std::vector<const char*> argv;
            for ( const auto& s : cmdArgs ) {
                argv.push_back( s.data() );
            }
            argv.push_back(NULL);
            argv.shrink_to_fit();

            const char* progname = argv[0];
            execvp( progname, const_cast<char* const *>(argv.data()) );
            close(fd[PIPE_WRITE]);
            exit(127);
        }

        // Read 2kB chunks from the child process.
        const int BUFSIZE = 2049;
        char buf[BUFSIZE] = {0}; 
    
        std::vector<std::string> cmd_result;

        close(fd[PIPE_WRITE]); // we won't write to stdout
        // read until closed, or error (0 or -1, respectively)
        for (int n = 0; (n = read(fd[PIPE_READ], buf, BUFSIZE-1)) > 0;){ 
            // std::cout << "Received "<< n <<" bytes from child process: " << std::endl;
            buf[n] = '\0';
            cmd_result.push_back(buf);
        }

        close(fd[PIPE_READ]);
        waitpid(pid, &status, 0);
        if (status != 0) return "";
        return std::string(boost::algorithm::join(cmd_result, ""));
    }
    #ifdef IN_OS_MAC
    namespace macos{        
        void get_platform_uuid(char * buf, int bufSize){
            io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMainPortDefault, "IOService:/");
            CFStringRef uuidCf = (CFStringRef) IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
            IOObjectRelease(ioRegistryRoot);
            CFStringGetCString(uuidCf, buf, bufSize, kCFStringEncodingMacRoman);
            CFRelease(uuidCf);
        }

        std::string get_uuid(void){
            char buf[512] = "";
            get_platform_uuid(buf, sizeof(buf));
            return std::string(buf);
        }
    }
    #endif
}
#endif