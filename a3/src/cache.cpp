#include "cache.h"
#include "page.h"
#include <string>

cache::cache()
{
    this->maxSize = 4;
    this->currentSize = 0;
}
cache::cache(int size)
{
    this->maxSize = size;
    this->currentSize = 0;
}

void cache::add(page pageToAdd)
{
    if(currentSize < 4) { // free space in the cache... yay...
        myCache.insert(myCache.begin(), pageToAdd);
        currentSize++;
    } else { // no free space in the cache... :( start with FIFO and go from there
        page pageToRemove = myCache.back();
        pageToRemove.writeToDisk();
        myCache.pop_back();
        myCache.insert(myCache.begin(), pageToAdd); // add the next entry in the cache
    }
}

// fake bool, as we want the index as well
int cache::contains(std::string filename)
{
    for(int i = 0; i < this->currentSize; i++) {
        if(myCache[i] == filename) {
            return i;
        }
    }
    return -1;
}
