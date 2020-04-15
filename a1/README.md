AUTHOR
        Sehej Sohal sssohal@ucsc.edu 1650056
NAME
        httpserver - HTTP server written from the ground up that can accept PUT and GET requests from any network client, such as CURL, WGET, netcat, and Postman.

INSTRUCTIONS for ./src folder

        make all - builds httpserver executable.
        make httpserver - builds httpserver executable.

        ./httpserver - starts httpserver on 127.0.0.1:8080.
        ./httpserver -p [PORT NUMBER] - starts httpserver on 127.0.0.1:PORT NUMBER.
        ./httpserver -p [PORT NUMBER] -a [ADDRESS] - starts httpserver on [ADDRESS]:[PORT NUMBER].

        make clean - removes executable.
        make spotless - removes all unneeded files.

INSTRUCTIONS for ./test folder

        python test.py <PORT#> - <PORT#> will default to 8080 unless specified.

DESCRIPTION
        Accepts GET and PUT requests for valid files. Handles files of any size. Supports HTTP codes 100, 200, 201, 400, 403, 404, and 500.

        curl -X GET [ADDRESS]:[PORT NUMBER]/filename ...
        curl [ADDRESS]:[PORT NUMBER]/filename --upload-file path/filename ...
        curl -d "..." -X PUT [ADDRESS]:[PORT NUMBER]/filename ...

CONSTRAINTS Filenames must be exactly 27 characters, and contain no other characters than [A-Z], [a-z], [0-9], -, and _.

        curl -X GET localhost:8080/YEbVmbnqCOKTR_XfVrVW4jMPBlX VALID
        curl -X GET localhost:8080/txtA$%b54v2*UJ^cw INVALID

        curl localhost:8080/YEbVmbnqCOKTR_XfVrVW4jMPBlX --upload-file path/YEbVmbnqCOKTR_XfVrVW4jMPBlX VALID
        curl localhost:8080/2aCiZpY3@@L7F4p6l --upload-file path/2aCiZpY3@@L7F4p6l INVALID

