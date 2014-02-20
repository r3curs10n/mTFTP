mTFTP
=====

TFTP (RFC 1350) client implementation for Networks Lab Assignment.

Usage
=====

Compilation: g++ -o tftpclient client.cpp -fpermissive -w

./tftpclient [-h hostname] [-p port] -r/-w filename

If not specified, hostname is taken to be 127.0.0.1 and port 69.

To write files make sure that the server program has sufficient permissions to write to its default directory.
