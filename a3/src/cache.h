#ifndef SRC_CLION_CACHE_H
#define SRC_CLION_CACHE_H

#include "page.h"
#include <string>
#include <vector>

class cache
{
  public:
    cache();
    cache(int size);
    void add(page pageToAdd);
    int contains(std::string filename);
    int getSize()
    {
        return this->currentSize;
    }
    page operator[](int i)
    {
        return this->myCache[i];
    }

  private:
    int maxSize;
    int currentSize;
    std::vector<page> myCache;
};

#endif // SRC_CLION_CACHE_H
