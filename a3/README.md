AUTHOR 

        Sehej Sohal sssohal@ucsc.edu 1650056
NAME

        httpserver -  HTTP server written from the ground up that can accept PUT and GET requests
        from any network client, such as CURL, WGET, netcat, and Postman. Utilizes a FIFO caching
        system and supports HEXDUMP logging.

INSTRUCTIONS for ./src folder

        make all - builds httpserver executable.
        make httpserver - builds httpserver executable.
        make clean - removes executable.
        make spotless - removes all unneeded files.

INSTRUCTIONS for launching httpserver

        args: -p(PORT), -a (ADDRESS), -l (LOGGING ENABLED/LOGFILE), -c (CACHING ENABLED)
                -p PORT (8080 if unspecified)
                -a ADDRESS (127.0.0.1 if unspecified)
                -l LOG_FILENAME (none if unspecified)
                -c
        Example Usage:
                ./httpserver -p 8080 -c
                ./httpserver -l logfile.txt -a 10.1.10.1
                ./httpserver -l logfile.txt -c
                ./httpserver -a 127.0.0.13 -l loggyboi.txt -p 8088

DESCRIPTION
        Accepts GET and PUT requests for valid files. Handles files of any size. Supports HTTP codes 100, 200, 201, 400, 403, 404, and 500.

        curl -X GET [ADDRESS]:[PORT NUMBER]/filename ...
        curl [ADDRESS]:[PORT NUMBER]/filename --upload-file path/filename ...
        curl -d "..." -X PUT [ADDRESS]:[PORT NUMBER]/filename ...

CONSTRAINTS 
        
        Filenames must be exactly 27 characters, and contain no other characters than [A-Z], [a-z], [0-9], -, and _.

        curl -X GET localhost:8080/YEbVmbnqCOKTR_XfVrVW4jMPBlX VALID
        curl -X GET localhost:8080/txtA$%b54v2*UJ^cw INVALID
        curl localhost:8080/YEbVmbnqCOKTR_XfVrVW4jMPBlX --upload-file path/YEbVmbnqCOKTR_XfVrVW4jMPBlX VALID
        curl localhost:8080/2aCiZpY3@@L7F4p6l --upload-file path/2aCiZpY3@@L7F4p6l INVALID

        Files that have recently been PUT to the server will not exist on disk until the server has been shut down with CTRL+C.
        This is a known contraint of FIFO caching. To access these files, please perform a GET request.
