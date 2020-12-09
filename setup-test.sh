
function generate_data() {
    mkdir data
    head -c 1M </dev/urandom > data/1M.bin
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
    ../build/server
}    

function start_clients() {
    clients=$1
    delay=$2
    for i in $(seq $clients)
    do
        echo Starting client $i...
        $(mkdir -p client$i; cd client$i; ../build/client -f 1M.bin &>/dev/null)&
        sleep $delay
    done
    wait
    echo All done.
}

time start_clients 20 0.1
