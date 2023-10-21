loopnum=5
sleeptime=1
clients=( 10 15 )

for c in "${clients[@]}"
do
	./loadtest.sh $c $loopnum $sleeptime >> data.txt
done

echo "set terminal png; set output 'Response_time.png';set title 'Average response time vs no. of clients';set xlabel 'no. of clients';set ylabel 'Average response time';plot 'data.txt' using 1:2 with linespoints title 'Avg response time'" | gnuplot
echo "set terminal png; set output 'Throughput.png';set title 'Overall throughput vs no. of clients';set xlabel 'no. of clients';set ylabel 'Overall throughput';plot 'data.txt' using 1:3 with linespoints title 'Overall throughput'" | gnuplot

rm data.txt
