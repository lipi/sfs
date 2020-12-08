
function generate_data() {
    mkdir data
    head -c 1M </dev/urandom > data/1M.bin
    head -c 10M </dev/urandom > data/10M.bin
    head -c 100M </dev/urandom > data/100M.bin
    head -c 1G </dev/urandom > data/1G.bin
}

function setup_traffic_control() {
    sudo tc qdisc add dev lo root handle 1: htb default 12 
    sudo tc class add dev lo parent 1:1 classid 1:12 htb rate 8Mbit ceil 8Mbit 
    sudo tc qdisc add dev lo parent 1:12 netem delay 1000ms
}

function reset_traffic_control() {
    sudo tc qdisc del dev lo root
}

function setup_tcp_tuning() {
    export BUFMAX=2097152
    sudo echo $BUFMAX > /proc/sys/net/core/wmem_max
    sudo echo $BUFMAX > /proc/sys/net/core/wmem_default 
    sudo echo $BUFMAX > /proc/sys/net/core/rmem_default 
    sudo echo $BUFMAX > /proc/sys/net/core/rmem_max

}
function start_server() {
    cd data
    
