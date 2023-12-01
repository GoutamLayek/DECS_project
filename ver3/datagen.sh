# #!/bin/bash

# if [ $# -lt 6 ];
# then
#     echo "Usage: $0 <serverip> <port no> <filename> <loopNum> <sleepTimeInSeconds> <TimeoutInSeconds>"
#     exit 1
# fi
# serverip=$1
# port=$2
# filename=$3
# loopNum=$4
# sleepTime=$5
# timeout=$6

# clients=( 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100)

# for c in "${clients[@]}"
# do
# 	./calculate.sh $serverip $port $filename $c $loopNum $sleepTime $timeout >> data.txt
# done

echo "set terminal png; set output 'Response_time.png';set title 'Average response time vs no. of clients';set xlabel 'no. of clients';set ylabel 'Average response time';plot 'data.txt' using 1:2 with linespoints title 'Avg response time'" | gnuplot
echo "set terminal png; set output 'Throughput.png';set title 'Overall throughput vs no. of clients';set xlabel 'no. of clients';set ylabel 'Overall throughput';plot 'data.txt' using 1:3 with linespoints title 'Overall throughput'" | gnuplot
echo "set terminal png; set output 'Request_rate.png';set title 'Request rate vs no. of clients';set xlabel 'no. of clients';set ylabel 'Request rate %';plot 'data.txt' using 1:4 with linespoints title 'Request rate vs no. of clients'" | gnuplot
echo "set terminal png; set output 'Success_rate.png';set title 'Success rate vs no. of clients';set xlabel 'no. of clients';set ylabel 'Success rate %';plot 'data.txt' using 1:5 with linespoints title 'Success rate vs no. of clients'" | gnuplot
echo "set terminal png; set output 'Timeout_rate.png';set title 'Timeout rate vs no. of clients';set xlabel 'no. of clients';set ylabel 'Timeout rate %';plot 'data.txt' using 1:6 with linespoints title 'Timeout rate vs no. of clients'" | gnuplot
echo "set terminal png; set output 'Error_rate.png';set title 'Error rate vs no. of clients';set xlabel 'no. of clients';set ylabel 'Error rate %';plot 'data.txt' using 1:7 with linespoints title 'Error rate vs no. of clients'" | gnuplot

#rm data.txt