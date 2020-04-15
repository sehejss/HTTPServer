#include "http_server.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

// basically done, need to just figure out how to write all cached files to disk on exit :)

class globalDestructor
{
  public:
    http_server *destructorToCall;
    globalDestructor()
    {
        destructorToCall = NULL;
    }
    void setDestructor(http_server *s)
    {
        destructorToCall = s;
    }
    void boom()
    {
        this->destructorToCall->~http_server();
    }
};

globalDestructor destructor = globalDestructor();

void my_handler(sig_atomic_t s)
{
    printf("\nCaught signal %d\n", s);
    destructor.boom();
    exit(1);
}

int main(int argc, char **argv)
{
    int port = 8080;
    std::string address = "127.0.0.1";
    std::string logFile = "";
    bool loggingEnabled = false;
    bool cachingEnabled = false;

    if(argc > 8) {
        printf("Too many arguments! Please specify -a (address) and/or -p (port) and/or -N (thread "
               "count)\n");
    }
    for(size_t i = 1; i < (size_t)argc; i++) {
        if(strcmp(argv[i], "-a") == 0) {
            address = std::string(argv[i + 1]);
        } else if(strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        } else if(strcmp(argv[i], "-l") == 0) {
            logFile = std::string(argv[i + 1]);
            loggingEnabled = true;
        } else if(strcmp(argv[i], "-c") == 0) {
            cachingEnabled = true;
        }
    }
    printf("Starting server on %s:%d.\n", address.c_str(), port);
    if(loggingEnabled) {
        printf("Logging enabled and written to %s\n", logFile.c_str());
    }
    if(cachingEnabled) {
        printf("Caching enabled\n");
        printf("Please shut down with CTRL+C to flush Cache to Disk.\n");
    }

    signal(SIGINT, my_handler);

    http_server myServer = http_server(port, address, logFile, loggingEnabled, cachingEnabled);
    destructor.setDestructor(&myServer);
    myServer.listen_and_accept();
    return 0;
}
