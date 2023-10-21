#!/bin/bash

if [ $# -ne 3 ]
then
	echo "usage: $0 <n_clients> <loopnum> <sleeptime>"
	exit 1
fi



serverip="127.0.0.1"
portno=9000
filename="code.c"
no_client=$1
loopnum=$2
sleeptime=$3



for(( i=0;i<$1;i++ ))
do
   ./client $serverip $portno $filename $2 $3 >> c$i.txt &
done

wait

response_time_sum=0
total_successes=0
throughput=0
avg_rsp_time=0
for file in c*.txt
do
	if [ -s $file ]; then #to check file is empty   if empty  out of if
		response_time_sum=$(awk -v sum="$response_time_sum" 'BEGIN {rsp_t=0;success=0} /Number of successful responses:/{ success=$NF } /Average response time:/{ rsp_t = $(NF - 1) } END { sum+=success*rsp_t; print sum}' $file)
		total_successes=$(awk -v total="$total_successes" 'BEGIN {success=0} /Number of successful responses:/{ success=$NF } END { total+=success; print total }' $file)
		throughput=$(awk -v thr="$throughput" 'BEGIN {tot_t=0;success=0} /Number of successful responses:/{ success=$NF } /Loop_time:/{ tot_t = $(NF - 1) } END { thr+=success/tot_t; print thr}' $file)
	fi
	rm $file
	
done

#echo "$response_time_sum $total_successes $throughput"
avg_rsp_time=$(awk -v total_resp_time="$response_time_sum" -v total_success="$total_successes" 'BEGIN { print total_resp_time/total_success}')
echo "$no_client $avg_rsp_time $throughput"


	
