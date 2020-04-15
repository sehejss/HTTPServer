#include <string>
#include "http_server.h"

int main(int argc, char** argv)
{
    int port = 0;
    std::string address = "";

    if(argc > 5)
    {
        printf("Too many arguments! Please specify either -a (address) and/or -p (port).");
    }
    for(size_t i = 1; i < (size_t)argc; i++)
    {
        if (strcmp(argv[i], "-a") == 0)
        {
            address = std::string(argv[i+1]);
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            port = atoi(argv[i+1]);
        }
    }
    http_server myServer = NULL;
    if(port != 0 && address == "")
    {   
        printf("a\n");
        myServer = http_server(port);
    }
    else if(port == 0 && address != "")
    {
        printf("b\n");
        fprintf(stderr, "Cannot specify custom address without first setting a port!\n");
        exit(1);
    }
    else if(port != 0 && address != "")
    {
        printf("c\n");
        myServer = http_server(port, address);
    }
    else
    {
        printf("d");
        myServer = http_server();
    }

    myServer.listen_and_accept();
    return 0;
}
