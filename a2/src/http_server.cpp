
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "http_server.h"

#include <iostream>
#include <regex>
#include <string>

#define LINE_TO_READ 20
#define HEX_LINE_SIZE 61
#define BYTE 8
#define LINE_TO_WRITE 69
#define LOG_SIZE 256
#define SIZE 4096
#define FILE_NAME_LENGTH 27

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t loggingLock = PTHREAD_MUTEX_INITIALIZER;

// unassociated function ptr for thread start routine
void *waitForWork(void *ptr)
{
    http_server *server;
    server = (http_server *)ptr;

    while(true) {
        pthread_mutex_lock(&mutex);
        while(server->syncQueue.empty()) {
            pthread_cond_wait(&cond, &mutex);
        }

        int client_fd = server->syncQueue.front();
        server->syncQueue.pop();
        pthread_mutex_unlock(&mutex);

        server->serve(client_fd);
    }
}

void asciiToHex(char *input, char *output)
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
  int thread_count,
  std::string logFile,
  bool loggingEnabled)
{
    this->port = custom_port;
    this->IP = custom_address;
    this->threads = thread_count;
    this->my_fd = socket(AF_INET, SOCK_STREAM, 0);
    this->addrlen = sizeof(address);
    this->log_offset = 0;
    this->loggingEnabled = loggingEnabled;
    this->myLogFile = logFile;

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

// add dispatcher functionality
void http_server::listen_and_accept()
{
    this->my_log_fd = open(this->myLogFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);

    if(listen(this->my_fd, 3) < 0) {
        perror("httpserver: error in listen\n");
        exit(EXIT_FAILURE);
    }

    pthread_t workerPool[this->threads];

    for(int i = 0; i < this->threads; i++) {
        pthread_create(&workerPool[i], NULL, waitForWork, (void *)this);
    }

    while(true) {
        int client =
          accept(this->my_fd, (struct sockaddr *)&this->address, (socklen_t *)&this->addrlen);
        if(client < 0) {
            perror("httpserver: error in accept\n");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&mutex);
        this->syncQueue.push(client);
        pthread_mutex_unlock(&mutex);
        // send signal to thread pool that there is work to do
        pthread_cond_signal(&cond);
    }
    close(this->my_log_fd);
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
    printf("httpserver: filesize: %ld\n", st.st_size);

    this->sendHeader(client, 200, "OK", st.st_size);
    this->writeHeaderToLog(200, "GET", 0, filename);

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
        this->writeHeaderToLog(400, "PUT", 0, filename);
        return;
    }
    // filename is valid, so we check if it's exists
    char fileToOpen[FILE_NAME_LENGTH];
    strcpy(fileToOpen, filename.c_str());

    int fileDescriptor = open(fileToOpen, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(fileDescriptor < 0) {
        warn("%s", fileToOpen);
        this->sendHeader(client, 500, "Internal Server Error", 0);
        this->writeHeaderToLog(500, "PUT", 0, filename);
        close(fileDescriptor);
        return;
    }
    // adding 1 to bodyStart in getBody() function causes an off-by-1 error, but this works...
    size_t bytesWritten = write(fileDescriptor, body + bodyStart + 1, bodyStop - bodyStart - 1);
    contentLength -= bytesWritten;

    this->echo(client, fileDescriptor, contentLength);
    close(fileDescriptor);
    this->sendHeader(client, 201, "Created", 0);
    this->writeHeaderToLog(201, "PUT", contentLength, filename);
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

void http_server::writeHeaderToLog(int statusCode,
  std::string method,
  off_t contentLength,
  std::string filename)
{
    if(!this->loggingEnabled) {
        return;
    }
    int old_offset;
    char header[LOG_SIZE];
    int length;
    if(statusCode < 400) {
        if(method == "PUT") {
            snprintf(header,
              LOG_SIZE,
              "%s %s length %ld\n",
              method.c_str(),
              filename.c_str(),
              contentLength);
            int lines = (contentLength / 20) + 1;
            length = strlen(header) + (LINE_TO_WRITE * lines) + (BYTE + 1);

        } else {
            snprintf(header,
              LOG_SIZE,
              "%s %s length %ld\n========\n",
              method.c_str(),
              filename.c_str(),
              contentLength);
            length = strlen(header);
        }
    } else {
        snprintf(header,
          LOG_SIZE,
          "FAIL: %s %s HTTP/1.1 --- response %d\n========\n",
          method.c_str(),
          filename.c_str(),
          statusCode);
        length = strlen(header);
    }

    old_offset = this->log_offset;
    pthread_mutex_lock(&loggingLock);
    this->log_offset = this->log_offset + length;
    pthread_mutex_unlock(&loggingLock);

    if(statusCode < 400 && method == "PUT") {
        int headerLength = strlen(header);
        pwrite(this->my_log_fd, header, headerLength, old_offset);
        writeFileToLog(filename, contentLength, old_offset + headerLength);
    } else {
        pwrite(this->my_log_fd, header, length, old_offset);
    }
}

void http_server::writeFileToLog(std::string filename, off_t contentLength, int oldOffset)
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

        bytesWritten = pwrite(this->my_log_fd, writeLine, LINE_TO_WRITE, oldOffset);

        oldOffset += LINE_TO_WRITE;

    } while(bytesRead > 0 && contentLength > 0);

    char lineBreak[] = "========\n";
    pwrite(this->my_log_fd, lineBreak, strlen(lineBreak), oldOffset);
    close(fileToRead);
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
