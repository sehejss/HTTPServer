#include "http_server.h"
#include <string>

int main(int argc, char **argv)
{
    int port = 8080;
    std::string address = "127.0.0.1";
    int threadCount = 4;
    std::string logFile = "";
    bool loggingEnabled = false;

    if(argc > 8) {
        printf("Too many arguments! Please specify -a (address) and/or -p (port) and/or -N (thread "
               "count)\n");
    }
    for(size_t i = 1; i < (size_t)argc; i++) {
        if(strcmp(argv[i], "-a") == 0) {
            address = std::string(argv[i + 1]);
        } else if(strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        } else if(strcmp(argv[i], "-N") == 0) {
            threadCount = atoi(argv[i + 1]);
        } else if(strcmp(argv[i], "-l") == 0) {
            logFile = std::string(argv[i + 1]);
            loggingEnabled = true;
        }
    }
    printf("Starting server on %s:%d with %d threads of execution.\n",
      address.c_str(),
      port,
      threadCount);
    if(loggingEnabled) {
        printf("Logging enabled and written to %s\n", logFile.c_str());
    }
    http_server myServer = http_server(port, address, threadCount, logFile, loggingEnabled);
    myServer.listen_and_accept();
    return 0;
}
