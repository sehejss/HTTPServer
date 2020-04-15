
#include "http_server.h"
#include "cache.h"
#include "page.h"
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINE_TO_READ 20
#define LOG_SIZE 256
#define SIZE 4096
#define FILE_NAME_LENGTH 27

void asciiToHex(const char *input, char *output)
{
    int loop = 0;
    int i = 0;
    while(input[loop] != '\0') {
        sprintf((char *)(output + i), " %02X", input[loop]);
        loop += 1;
        i += 3;
    }
    output[i++] = '\0';
}

http_server::http_server(int custom_port,
  std::string custom_address,
  std::string logFile,
  bool loggingEnabled,
  bool cachingEnabled)
{
    this->port = custom_port;
    this->IP = custom_address;
    this->my_fd = socket(AF_INET, SOCK_STREAM, 0);
    this->addrlen = sizeof(address);
    this->loggingEnabled = loggingEnabled;
    this->myLogFile = logFile;
    this->cachingEnabled = cachingEnabled;
    this->fileCache = cache();

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

http_server::~http_server()
{
    printf("Shutting down server...\n");
    close(this->my_fd);
    close(this->my_bind_fd);
    printf("Writing cache to Disk...\n");
    this->writeCacheToDisk();
}

void http_server::listen_and_accept()
{
    this->my_log_fd = open(this->myLogFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);

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
    close(this->my_log_fd);
}

void http_server::serve(int client)
{
    char buffer[SIZE] = { 0 };
    char seperateBuffer[SIZE] = { 0 };

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
    bool sepBuffer = false;
    if(expectation == "") {
        bodyStart = this->getBody(buffer);
    } else {
        this->sendHeader(client, 100, "Continue", 0);
        bodyStop = recv(client, seperateBuffer, SIZE, 0);
        bodyStart = this->getBody(seperateBuffer);
        sepBuffer = true;
    }

    if(http_method == "GET") {
        printf("httpserver: GET message received\n");
        this->handle_get(client, filenameString);
    } else if(http_method == "PUT") {
        printf("httpserver: PUT message received\n");
        this->handle_put(client,
          filenameString,
          contentLength,
          (sepBuffer ? seperateBuffer : buffer),
          bodyStart,
          bodyStop);
    } else {
        printf("httpserver: Unsupported Method received\n");
        this->sendHeader(client, 403, "Forbidden", 0);
        this->writeHeaderToLog(403, "NULL", 0, filenameString);
    }
    printf("httpserver: Message Handled\n");
    close(client);
    return;
}

void http_server::handle_get(int client, std::string filename)
{
    if(!this->check_name_validity(filename)) {
        this->sendHeader(client, 400, "Bad Request", 0);
        this->writeHeaderToLog(400, "GET", 0, filename);
        close(client);
        return;
    }
    if(this->cachingEnabled) {
        int validIndex = this->fileCache.contains(filename);
        if(validIndex >= 0) {
            this->writeHeaderToLog(200, "GET", 0, filename, " [was in cache]");
            std::string content = this->fileCache[validIndex].getContent();
            const char *contentCString = content.c_str();
            size_t bytesToWrite = strlen(contentCString);
            do {
                size_t bytesWritten = write(client, contentCString, bytesToWrite);
                bytesToWrite -= bytesWritten;
            } while(bytesToWrite > 0);
        } else {
            // filename is valid, so we check if it's exists
            char fileToOpen[FILE_NAME_LENGTH];
            strcpy(fileToOpen, filename.c_str());

            int fileDescriptor = open(fileToOpen, O_RDONLY);

            if(fileDescriptor < 0) {
                if(errno == ENOENT) {
                    warn("%s", fileToOpen);
                    this->sendHeader(client, 404, "Not Found", 0);
                    this->writeHeaderToLog(404, "GET", 0, filename);

                } else if(errno == EACCES) {
                    warn("%s", fileToOpen);
                    this->sendHeader(client, 403, "Forbidden", 0);
                    this->writeHeaderToLog(403, "GET", 0, filename);
                } else {
                    warn("%s", fileToOpen);
                    this->sendHeader(client, 500, "Internal Server Error", 0);
                    this->writeHeaderToLog(500, "GET", 0, filename);
                }
                close(fileDescriptor);
                return;
            }

            struct stat st;
            fstat(fileDescriptor, &st);
            off_t contentLength = st.st_size;
            printf("httpserver: filesize: %ld\n", contentLength);

            this->sendHeader(client, 200, "OK", contentLength);
            this->writeHeaderToLog(200, "GET", 0, filename, " [was not in cache]");
            echo(fileDescriptor, client, contentLength);
            close(fileDescriptor);
            page pageToAdd = page(filename, contentLength);
            this->fileCache.add(pageToAdd);
        }
    } else {
        // filename is valid, so we check if it's exists
        char fileToOpen[FILE_NAME_LENGTH];
        strcpy(fileToOpen, filename.c_str());

        int fileDescriptor = open(fileToOpen, O_RDONLY);

        if(fileDescriptor < 0) {
            if(errno == ENOENT) {
                warn("%s", fileToOpen);
                this->sendHeader(client, 404, "Not Found", 0);
                this->writeHeaderToLog(404, "GET", 0, filename);

            } else if(errno == EACCES) {
                warn("%s", fileToOpen);
                this->sendHeader(client, 403, "Forbidden", 0);
                this->writeHeaderToLog(403, "GET", 0, filename);
            } else {
                warn("%s", fileToOpen);
                this->sendHeader(client, 500, "Internal Server Error", 0);
                this->writeHeaderToLog(500, "GET", 0, filename);
            }
            close(fileDescriptor);
            return;
        }

        struct stat st;
        fstat(fileDescriptor, &st);
        off_t contentLength = st.st_size;
        printf("httpserver: filesize: %ld\n", contentLength);

        this->sendHeader(client, 200, "OK", contentLength);
        this->writeHeaderToLog(200, "GET", 0, filename);
        echo(fileDescriptor, client, st.st_size);
        close(fileDescriptor);
    }
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
        this->writeHeaderToLog(400, "PUT", 0, filename);
        return;
    }
    if(this->cachingEnabled) {
        int validIndex = this->fileCache.contains(filename);
        std::string temp = body;
        std::string cppBody = temp.substr(bodyStart + 1, bodyStop - bodyStart - 1);
        if(validIndex >= 0) {
            if((off_t)cppBody.length() >= contentLength) {
                this->fileCache[validIndex].fillFromString(cppBody);
            } else {
                this->fileCache[validIndex].fillFromClient(
                  client, cppBody, contentLength - cppBody.length());
                this->writeHeaderToLog(201, "PUT", contentLength, filename, " [was in cache]");
            }
        } else {
            if((off_t)cppBody.length() >= contentLength) {
                page pageToAdd = page(filename, cppBody);
                this->fileCache.add(pageToAdd);
            } else {
                page pageToAdd = page(filename, client, cppBody, contentLength - cppBody.length());
                this->fileCache.add(pageToAdd);
            }
            this->writeHeaderToLog(201, "PUT", contentLength, filename, " [was not in cache]");
        }
    } else {
        int fileDescriptor = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
        if(fileDescriptor < 0) {
            warn("%s", filename.c_str());
            this->sendHeader(client, 500, "Internal Server Error", 0);
            this->writeHeaderToLog(500, "PUT", 0, filename);
            close(fileDescriptor);
            return;
        }
        size_t bytesWritten = write(fileDescriptor, body + bodyStart + 1, bodyStop - bodyStart - 1);
        off_t contentLengthToEcho = contentLength;
        contentLengthToEcho -= bytesWritten;
        if(contentLengthToEcho >= 0) {
            this->echo(client, fileDescriptor, (contentLengthToEcho > 0) ? contentLengthToEcho : 0);
        }
        close(fileDescriptor);
        this->writeHeaderToLog(201, "PUT", contentLength, filename);
    }
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

void http_server::writeCacheToDisk()
{
    for(int i = 0; i < this->fileCache.getSize(); i++) {
        this->fileCache[i].writeToDisk();
    }
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

void http_server::writeHeaderToLog(int statusCode,
  std::string method,
  off_t contentLength,
  std::string filename,
  std::string wasInCache)
{
    if(!this->loggingEnabled) {
        return;
    }
    char header[LOG_SIZE];
    int length;
    if(statusCode < 400) {
        if(method == "PUT") {
            snprintf(header,
              LOG_SIZE,
              "%s %s length %ld%s\n",
              method.c_str(),
              filename.c_str(),
              contentLength,
              wasInCache.c_str());
        } else {
            snprintf(header,
              LOG_SIZE,
              "%s %s length %ld%s\n========\n",
              method.c_str(),
              filename.c_str(),
              contentLength,
              wasInCache.c_str());
        }
    } else {
        snprintf(header,
          LOG_SIZE,
          "FAIL: %s %s HTTP/1.1 --- response %d\n========\n",
          method.c_str(),
          filename.c_str(),
          statusCode);
    }
    length = strlen(header);
    if(statusCode < 400 && method == "PUT") {
        write(this->my_log_fd, header, length);
        if(this->cachingEnabled) {
            writeCachedFileToLog(filename);
        } else {
            writeFileToLog(filename, contentLength);
        }
    } else {
        write(this->my_log_fd, header, length);
    }
}

void http_server::writeFileToLog(std::string filename, off_t contentLength)
{
    int fileToRead = open(filename.c_str(), O_RDONLY);
    if(fileToRead < 0) {
        warn("%s", filename.c_str());
    }
    char line[LINE_TO_READ];
    int bytesRead;
    int byteCount = 0;
    do {
        size_t bytesWritten;

        bytesRead = read(fileToRead,
          line,
          (contentLength < LINE_TO_READ ? contentLength
                                        : LINE_TO_READ)); // hopefully reads 20 characters
        contentLength -= LINE_TO_READ;
        char writeLine[LOG_SIZE];
        int len = strlen(line);
        char hex_string[(len * 3) + 1];

        asciiToHex(line, hex_string);

        snprintf(writeLine, LOG_SIZE, "%08d%s\n", byteCount, hex_string);
        byteCount += LINE_TO_READ;

        bytesWritten = write(this->my_log_fd, writeLine, strlen(writeLine));

    } while(bytesRead > 0 && contentLength > 0);

    char lineBreak[] = "========\n";
    write(this->my_log_fd, lineBreak, strlen(lineBreak));
    close(fileToRead);
}

void http_server::writeCachedFileToLog(std::string filename)
{
    int validIndex = this->fileCache.contains(
      filename); // don't need to error check here as it should always be valid.
    std::string contents = this->fileCache[validIndex].getContent();
    size_t contentLength = contents.length();

    int byteCount = 0;
    for(size_t i = 0; i < contentLength; i += 20) {
        std::string temp = contents.substr(i, 20);
        char hex_string[(temp.length() * 3) + 1];
        asciiToHex(temp.c_str(), hex_string);

        char writeLine[LOG_SIZE];
        snprintf(writeLine, LOG_SIZE, "%08d%s\n", byteCount, hex_string);
        byteCount += LINE_TO_READ;
        write(this->my_log_fd, writeLine, strlen(writeLine));
    }
    char lineBreak[] = "========\n";
    write(this->my_log_fd, lineBreak, strlen(lineBreak));
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

// wrapper so that we may initialize a thread with this function
// possibly also be a worker function
