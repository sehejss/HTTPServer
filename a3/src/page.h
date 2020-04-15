#ifndef SRC_CLION_PAGE_H
#define SRC_CLION_PAGE_H

#include <string>

class page
{
  public:
    page(std::string filename, off_t contentLength);
    page(std::string filename, int clientFd, std::string initialBody, off_t contentLength);
    page(std::string filename, std::string initialBody);
    void fillFromClient(int fileDescriptor, std::string initialBody, off_t contentLength);
    void fillFromString(std::string initialBody);
    void writeToDisk();

    bool operator==(std::string nameToCheck)
    {
        return (this->filename == nameToCheck);
    }

    std::string getFilename()
    {
        return this->filename;
    };
    std::string getContent()
    {
        return this->content;
    };

  private:
    std::string filename;
    std::string content;
};

#endif // SRC_CLION_PAGE_H
