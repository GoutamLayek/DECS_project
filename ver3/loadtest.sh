#!/bin/bash

if [ $# -lt 7 ];
then
    echo "Usage: $0 <serverip> <port no> <filename> <numClients> <loopNum> <sleepTimeInSeconds> <TimeoutInSeconds>"
    exit 1
fi

serverip=$1
port=$2
filename=$3
no_of_clients=$4
loopNum=$5
sleepTime=$6
timeout=$7

for ((i = 1; i <= $no_of_clients; i++));
do
    ./client $serverip $port $filename $loopNum $sleepTime $timeout >> client$i.txt &

done
wait
