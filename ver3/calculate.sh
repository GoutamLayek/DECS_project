#!/bin/bash

if [ $# -lt 7 ];
then
    echo "Usage: $0 <serverip> <port no> <filename> <numClients> <loopNum> <sleepTimeInSeconds> <TimeoutInSeconds>"
    exit 1
fi
serverip=$1
port=$2
filename=$3
clients=$4
loopNum=$5
sleepTime=$6
timeout=$7
gcc -o client client.c
start_time=$(date +%s.%N)
./loadtest.sh $serverip $port $filename $clients $loopNum $sleepTime $timeout
end_time=$(date +%s.%N)
total_time=$(awk -v e="$end_time" -v s="$start_time" 'BEGIN { print e-s}')

# consolidate 
cat client*.txt > all_clients.txt

awk -f "final.awk" all_clients.txt > global_data.txt
# rm -f client*.txt
# rm -f all_clients.txt

avg_response_time=$(awk '{print $1}' global_data.txt)
total_requests=$(awk '{print $2}' global_data.txt)
total_success=$(awk '{print $3}' global_data.txt)
total_timeouts=$(awk '{print $4}' global_data.txt)

throughput=$(awk -v t="$total_time" -v s="$total_success" 'BEGIN { print s/t}')
req_rate=$(awk -v t="$total_time" -v r="$total_requests" 'BEGIN { print r/t}')
success_percent=$(awk -v r="$total_requests" -v s="$total_success" 'BEGIN { print 100 * s/r}')
timeout_percent=$(awk -v r="$total_requests" -v to="$total_timeouts" 'BEGIN { print 100*to/r}')
error_percent=$(awk -v r="$total_requests" -v s="$total_success" 'BEGIN { print 100*(r-s)/r}')

echo "$clients $avg_response_time $throughput $req_rate $success_percent $timeout_percent $error_percent"
# rm global_data.txt