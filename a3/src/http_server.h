/// \file

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include "cache.h"
#include <netinet/in.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

class http_server
{
  public:
    http_server(int custom_port,
      std::string custom_address,
      std::string logFile,
      bool logging_enabled,
      bool caching_enabled);
    ~http_server();
    void listen_and_accept();
    void serve(int client);
    void writeCacheToDisk();

  private:
    struct sockaddr_in address;
    int port;
    std::string IP;
    int addrlen;
    int my_fd;
    int my_bind_fd;
    int my_log_fd;
    std::string myLogFile;
    bool loggingEnabled;
    cache fileCache;
    bool cachingEnabled;

    void handle_get(int client, std::string filename);
    void handle_put(int client,
      std::string filename,
      off_t contentLength,
      char *body,
      size_t bodyStart,
      size_t bodyStop);
    void echo(int readFileFd, int clientFd, off_t contentLength);
    void sendHeader(int clientFd, int statusCode, std::string statusMessage, off_t fileSize);
    void writeHeaderToLog(int statusCode,
      std::string method,
      off_t contentLength,
      std::string filename,
      std::string wasInCache = "");
    void writeFileToLog(std::string filename, off_t contentLength);
    void writeCachedFileToLog(std::string filename);

    bool check_name_validity(std::string filename);

    std::string getExpectation(std::string header);
    std::string getMethod(std::string header);
    std::string getFilename(std::string header);

    off_t getContentLength(std::string header);

    size_t getBody(char *buffer);
};

#endif