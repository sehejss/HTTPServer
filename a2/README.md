AUTHOR 

        Sehej Sohal sssohal@ucsc.edu 1650056
NAME

        httpserver - Multithreaded HTTP server written from the ground up that can accept PUT and GET requests
        from any network client, such as CURL, WGET, netcat, and Postman. Utilizes a user-specified amount of
        threads and supports contiguous logging.

INSTRUCTIONS for ./src folder

        make all - builds httpserver executable.
        make httpserver - builds httpserver executable.
        make clean - removes executable.
        make spotless - removes all unneeded files.

INSTRUCTIONS for launching httpserver

        args:
                -p PORT (8080 if unspecified)
                -a ADDRESS (127.0.0.1 if unspecified)
                -l LOG_FILENAME (none if unspecified)
                -N THREAD_COUNT (4 if unspecified)
        Example Usage:
                ./httpserver -p 8080 -N 4
                ./httpserver -l logfile.txt -a 10.1.10.1
                ./httpserver -N 4 -l logfile.txt
                ./httpserver -a 127.0.0.1 -N 3 -l loggyboi.txt -p 8088

DESCRIPTION
        Accepts GET and PUT requests for valid files. Handles files of any size. Supports HTTP codes 100, 200, 201, 400, 403, 404, and 500. Can handle multiple concurrect requests at once, without interweaving.

        curl -X GET [ADDRESS]:[PORT NUMBER]/filename ...
        curl [ADDRESS]:[PORT NUMBER]/filename --upload-file path/filename ...
        curl -d "..." -X PUT [ADDRESS]:[PORT NUMBER]/filename ...

CONSTRAINTS Filenames must be exactly 27 characters, and contain no other characters than [A-Z], [a-z], [0-9], -, and _.

        curl -X GET localhost:8080/YEbVmbnqCOKTR_XfVrVW4jMPBlX VALID
        curl -X GET localhost:8080/txtA$%b54v2*UJ^cw INVALID

        curl localhost:8080/YEbVmbnqCOKTR_XfVrVW4jMPBlX --upload-file path/YEbVmbnqCOKTR_XfVrVW4jMPBlX VALID
        curl localhost:8080/2aCiZpY3@@L7F4p6l --upload-file path/2aCiZpY3@@L7F4p6l INVALID

KNOWN ISSUES

        Contiguous logging is a bit buggy, I couldn't get it working perfectly. If the last line of characters
        is less than 20, say 15, the last 5 characters from the previous line will be printed again. I wasn't 
        able to get the line to truncate, as there was some issue with my conversion function.

        I have uncsynchronized printf logging, but that is not "official" and is purely used for whoever is
        running the server. It should be ignored.
