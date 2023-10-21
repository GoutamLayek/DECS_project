#!/bin/bash

if [ $# -lt 4 ];
then
    echo "Usage: $0 <numClients> <loopNum> <sleepTimeInSeconds> <TimeoutInSeconds>"
    exit 1
fi
gcc -o client client.c
file="code.c"
ip="127.0.0.1"
port=9000
for ((i = 1; i <= $1; i++));
do
    ./client $ip $port $file $2 $3 $4 >> output_$i.txt &

done
wait
sum_of_response_time=0
total_success=0
total_errors=0
total_timeouts=0
total_req=0
sum_of_loop_time=0
for file in output_*.txt;
do
    total_req=$(awk -v )
done