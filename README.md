# SFS - Simple File Server

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


# Build Instructions

    mkdir build
    cd build
    cmake ..
    make
    
# Usage

    Usage: ./server [-p <port>] [-d <directory>]
           -p <port> - TCP port to listen on (default: 50000)
           -d <directory> - directory to serve files from (default: current)

    Usage: ./client [-h <hostname>] [-p <port>] -f <filename>
           -h <host> - hostname to connect (default: localhost)
           -p <port> - TCP port to connect (default: 50000)
           -f <filename> - file to download

# Testing on Linux

Create files with random data:

    mkdir data
    head -c 1M </dev/urandom > data/1M.bin
    
Start the server:

    cd data
    ../server

Set up traffic shaping on loopback interface, to use 8Mbit/sec with 1000msec round trip (BDP = 1MByte):

    sudo tc qdisc add dev lo root handle 1: htb default 12 
    sudo tc class add dev lo parent 1:1 classid 1:12 htb rate 8Mbit ceil 8Mbit 
    sudo tc qdisc add dev lo parent 1:12 netem delay 1000ms

If you want to simulate lossy radio link:

    sudo tc qdisc add dev lo parent 1:12 netem delay 1000ms loss 1%

Check your traffic control settings:

    sudo tc class show dev lo
    sudo tc qdisc show dev lo

Delete settings:

    sudo tc qdisc del dev lo root

Start concurrent downloads:

    mkdir client1; cd client1
    time ../client  -h localhost -f 1M.data

    mkdir client2; cd client2
    time ../client  -h localhost -f 1M.data

Check your kernel's transmit/receive buffer sizes:

    cat /proc/sys/net/core/wmem_max
    cat /proc/sys/net/core/wmem_default
    cat /proc/sys/net/core/rmem_max
    cat /proc/sys/net/core/rmem_default

    cat /proc/sys/net/ipv4/tcp_wmem
    cat /proc/sys/net/ipv4/tcp_rmem

Allow large transmit/receive buffers:

    export BUFMAX=2097152
    sudo echo $BUFMAX > /proc/sys/net/core/wmem_max
    sudo echo $BUFMAX > /proc/sys/net/core/wmem_default 
    sudo echo $BUFMAX > /proc/sys/net/core/rmem_default 
    sudo echo $BUFMAX > /proc/sys/net/core/rmem_max

# Memcheck

    valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ../build/server

# Monitoring

    sudo apt install speedometer
    speedometer -r lo
