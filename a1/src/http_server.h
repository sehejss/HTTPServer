/// \file

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <string>

class http_server {
public:
    http_server(int custom_port=8080, std::string custom_address="127.0.0.1");
    void listen_and_accept();
private:
    struct sockaddr_in address;
    int port;
    std::string IP;
    int addrlen;
    int my_fd;
    int my_bind_fd;

    void serve(int client);
    void handle_get(int client, std::string filename);
    void handle_put(int client, std::string filename, off_t contentLength, char* body, size_t bodyStart, size_t bodyStop);
    void echo(int readFileFd, int clientFd, off_t contentLength);
    void sendHeader(int clientFd, int statusCode, std::string statusMessage, off_t fileSize);

    bool check_name_validity(std::string filename);

    std::string getExpectation(std::string header);
    std::string getMethod(std::string header);
    std::string getFilename(std::string header);

    off_t getContentLength(std::string header);

    size_t getBody(char* buffer);
};

#endif