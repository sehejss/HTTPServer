#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "page.h"

#include <string>

#define SIZE 4096

page::page(std::string filename, off_t contentLength)
{
    this->filename = filename;
    int fileDescriptor = open(filename.c_str(), O_RDONLY);
    this->fillFromClient(fileDescriptor, "", contentLength);
    close(fileDescriptor);
}

page::page(std::string filename, int clientFd, std::string initialBody, off_t contentLength)
{
    this->filename = filename;
    this->fillFromClient(clientFd, initialBody, contentLength);
}

page::page(std::string filename, std::string initialBody)
{
    this->filename = filename;
    this->fillFromString(initialBody);
}

// add a contentLength here, we don't want to do a read when theres no data left!
void page::fillFromClient(int fileDescriptor, std::string initialBody, off_t contentLength)
{
    this->content = initialBody;
    char myBuffer[SIZE];
    size_t bytesRead;
    do {
        bytesRead = read(fileDescriptor, myBuffer, (contentLength < SIZE ? contentLength : SIZE));
        contentLength -= SIZE;
        if(bytesRead > 0) {
            std::string temp = myBuffer;
            this->content += temp;
        }
    } while(bytesRead > 0 && contentLength > 0);
}

void page::fillFromString(std::string initialBody)
{
    this->content = initialBody;
}

void page::writeToDisk()
{
    int fd = open(this->filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(fd < 0) {
        perror("page: error in writeToDisk");
    }
    const char *contentCString = this->content.c_str();
    size_t bytesToWrite = strlen(contentCString);
    do {
        size_t bytesWritten = write(fd, contentCString, bytesToWrite);
        bytesToWrite -= bytesWritten;
    } while(bytesToWrite > 0);
    close(fd);
}
