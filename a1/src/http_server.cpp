#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#include "http_server.h"

#include <iostream>
#include <regex>
#include <string>


#define STDOUT 1
#define SIZE 4096
#define FILE_NAME_LENGTH 27

http_server::http_server(int custom_port, std::string custom_address)
{
    this->port = custom_port;
    this->IP = custom_address;
    this->my_fd = socket(AF_INET, SOCK_STREAM, 0);
    this->addrlen = sizeof(address);

    if(this->my_fd < 0) {
        perror("httpserver: cannot create socket\n");
        exit(1);
    } 

    memset((char *)&this->address, 0, sizeof(this->address));
    this->address.sin_family = AF_INET;
    this->address.sin_addr.s_addr = inet_addr(custom_address.c_str());
    this->address.sin_port = htons(this->port);
    this->my_bind_fd = ::bind(this->my_fd, (struct sockaddr *)&address, sizeof(address));
    if(this->my_bind_fd < 0) {
        perror("httpserver: bind failed\n");
        exit(1);
    }
}

void http_server::listen_and_accept()
{
    if(listen(this->my_fd, 3) < 0) {
        perror("httpserver: error in listen\n");
        exit(EXIT_FAILURE);
    }

    while(69) {
        int client =
          accept(this->my_fd, (struct sockaddr *)&this->address, (socklen_t *)&this->addrlen);

        if(client < 0) {
            perror("httpserver: error in accept\n");
            exit(EXIT_FAILURE);
        }
        this->serve(client);
        close(client);
    }
}

void http_server::serve(int client)
{
    char buffer[SIZE] = { 0 };

    size_t valread = recv(client, buffer, SIZE, 0);
    if(valread < 0) {
        printf("httpserver: no bytes to read\n");
    }

    std::string httpHeader = std::string(buffer);
    std::string http_method = this->getMethod(httpHeader);
    std::string filenameString = this->getFilename(httpHeader);
    off_t contentLength = this->getContentLength(httpHeader);
    std::string expectation = this->getExpectation(httpHeader);

    size_t bodyStart = -1;
    size_t bodyStop = valread;
    if(expectation == "") {
        bodyStart = this->getBody(buffer);
    } else {
        this->sendHeader(client, 100, "Continue", 0);
        bodyStop = recv(client, buffer, SIZE, 0);
        bodyStart = this->getBody(buffer);
    }

    if(http_method == "GET") {
        printf("httpserver: GET message received\n");
        this->handle_get(client, filenameString);
    } else if(http_method == "PUT") {
        printf("httpserver: PUT message received\n");
        this->handle_put(client, filenameString, contentLength, buffer, bodyStart, bodyStop);
    } else {
        printf("httpserver: Unsupported Method received\n");
        this->sendHeader(client, 403, "Forbidden", 0);
    }
    printf("httpserver: Message Handled\n");
    return;
}

void http_server::handle_get(int client, std::string filename)
{
    if(!this->check_name_validity(filename)) {
        this->sendHeader(client, 400, "Bad Request", 0);
        close(client);
        return; 
    }
    // filename is valid, so we check if it's exists
    char fileToOpen[FILE_NAME_LENGTH];
    strcpy(fileToOpen, filename.c_str());

    int fileDescriptor = open(fileToOpen, O_RDONLY);

    if(fileDescriptor < 0) {
        if(errno == ENOENT) {
            warn("%s", fileToOpen);
            this->sendHeader(client, 404, "Not Found", 0);
        } else if(errno == EACCES) {
            warn("%s", fileToOpen);
            this->sendHeader(client, 403, "Forbidden", 0);
        } else {
            warn("%s", fileToOpen);
            this->sendHeader(client, 500, "Internal Server Error", 0);
        }
        close(fileDescriptor);
        return; // construct 404 not found
    }

    struct stat st;
    fstat(fileDescriptor, &st);
    printf("httpserver: filesize: %ld\n", st.st_size);

    this->sendHeader(client, 200, "OK", st.st_size);

    echo(fileDescriptor, client, st.st_size);
    close(fileDescriptor);
    return;
}

void http_server::handle_put(int client,
  std::string filename,
  off_t contentLength,
  char *body,
  size_t bodyStart,
  size_t bodyStop)
{
    if(!this->check_name_validity(filename)) {
        this->sendHeader(client, 400, "Bad Request", 0);
        return; 
    }
    // filename is valid, so we check if it's exists
    char fileToOpen[FILE_NAME_LENGTH];
    strcpy(fileToOpen, filename.c_str());

    int fileDescriptor = open(fileToOpen, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(fileDescriptor < 0) {
        warn("%s", fileToOpen);
        this->sendHeader(client, 500, "Internal Server Error", 0);
        close(fileDescriptor);
        return;
    }
    //adding 1 to bodyStart in getBody() function causes an off-by-1 error, but this works...
    size_t bytesWritten = write(fileDescriptor, body + bodyStart + 1, bodyStop - bodyStart - 1);
    contentLength -= bytesWritten;

    this->echo(client, fileDescriptor, contentLength);
    close(fileDescriptor);
    this->sendHeader(client, 201, "Created", 0);
}

void http_server::echo(int readFileFd, int clientFd, off_t contentLength)
{
    char myBuffer[SIZE];
    size_t bytesRead;
    do {
        size_t bytesWritten;

        bytesRead = read(readFileFd, myBuffer, (contentLength < SIZE ? contentLength : SIZE));
        contentLength -= SIZE;
        bytesWritten = write(clientFd, myBuffer, bytesRead);

    } while(bytesRead > 0 && contentLength > 0);
}

void http_server::sendHeader(int clientFd,
  int statusCode,
  std::string statusMessage,
  off_t fileSize)
{
    char header[SIZE];
    snprintf(header,
      SIZE,
      "HTTP/1.1 %d %s\r\nContent-Length: %ld\r\n\r\n",
      statusCode,
      statusMessage.c_str(),
      fileSize);
    printf("httpserver: Header to send:\n%s", header);
    send(clientFd, header, strlen(header), 0);
    return;
}

bool http_server::check_name_validity(std::string filename)
{
    std::regex re("([a-zA-Z0-9-_]{27})");

    if(std::regex_match(filename, re)) {
        printf("httpserver: filename is valid\n");
        return true;
    }
    printf("httpserver: filename is invalid, must contain A-Z, a-z, 0-9, -, or _, and be exactly "
           "27 characters\n");
    return false;
}

std::string http_server::getExpectation(std::string header)
{
    std::string result = "";
    std::regex re("(Expect: )([0-9]*-[a-zA-Z]*)");
    std::smatch match;

    if(std::regex_search(header, match, re)) {
        result = match.str(2);
    }
    return result;
}

std::string http_server::getMethod(std::string header)
{
    std::string result = "";
    std::regex re("(GET|PUT)");
    std::smatch match;

    if(std::regex_search(header, match, re)) {
        result = match.str(1);
    }
    return result;
}

std::string http_server::getFilename(std::string header)
{
    std::string result = "";
    std::regex re("(GET|PUT)( /)(.*)( HTTP)");
    std::smatch match;

    if(std::regex_search(header, match, re)) {
        result = match.str(3);
    }
    return result;
}

off_t http_server::getContentLength(std::string header)
{
    off_t contentLength = -1;
    std::string result = "";
    std::regex re("(Content-Length: )([0-9]*)");
    std::smatch match;

    if(std::regex_search(header, match, re)) {
        result = match.str(2);
    }
    if(result != "") {
        contentLength = std::stoull(result);
    }
    return contentLength;
}

size_t http_server::getBody(char *buffer)
{
    for(size_t i = 0; i < SIZE - 3; i++) {
        if((buffer[i] == '\r') && (buffer[i + 1] == '\n') && (buffer[i + 2] == '\r')
           && (buffer[i + 3] == '\n')) {
            return i + 4;
        }
    }
    return -1;
}



