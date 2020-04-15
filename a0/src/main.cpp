#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <stdint.h>

/*
 * Define SIZE as 32Kb as it is our maximum buffer size
 * Define STDIN/STDOUT as their attached file descriptors
*/
#define SIZE 32768
#define STDIN 0
#define STDOUT 1

void echo(int fd_input);

int main(int argc, char** argv) {
    if (argc == 1) {
        echo(STDIN);
    }

    for(size_t i = 1; i < (size_t)argc; i++) {

        char* filename = argv[i];

        if (!strcmp(filename, "-")) {
            echo(STDIN);
            continue;
        }

        int8_t fileDescriptor = open(filename, O_RDONLY);
                
        if(fileDescriptor < 0) {
            warn("%s", filename);
            continue;
        }
        echo(fileDescriptor);
        if(errno != 0) {
            warn("%s", filename);
        }
        close(fileDescriptor);
    }
    return 0; 
} 

/*
 * Name: echo
 * Functionality: echos data from a file to standard output
 * Input: Integer value of a file descriptor(0-255)
 * Output: 0 for sucessful echo, 1-254 for errorcodes. Currently only ISDIR is checked.
 * Example Usage: echoing data to stdout/stdin/stderr from a file
*/
void echo(int fd_input) {
    char myBuffer[SIZE];
    size_t bytesRead;
    do {
        size_t bytesWritten;
        bytesRead = read(fd_input, myBuffer, SIZE);
        if (errno != 0) {
            return;
        }
        bytesWritten = write(STDOUT, myBuffer, bytesRead);
    }
    while (bytesRead > 0);
}