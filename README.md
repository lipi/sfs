# fts

Simple file server via TCP/IP
 
 * serves files for reading
 * can handle multiple concurrent clients
 * no authentication or encryption
 * serves files from a single directory (no recursion)
 * optimized for narrow bandwidth, high latency
 * no dependencies
 * C99   
 * POSIX


# Protocol

Client sends filename it wants download, server responds with data and closes connection.


            client                              server
            
               |                                   |
               |    filename length (8 bytes)      |
               |---------------------------------> |
               |                                   |
               |      filename (N bytes)           |
               |---------------------------------> |
               |                                   |
               |               data                |
               | <---------------------------------|
               |               data                |
               | <---------------------------------|
               |               data                |
               | <---------------------------------|
               |               data                |
               | <---------------------------------|
               |                                   |
               |                                   |
               |         close connection          |
               | <---------------------------------|
               |                                   |
