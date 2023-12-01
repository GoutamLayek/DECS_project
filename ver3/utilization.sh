#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <cpu_usage.txt>"
    exit 1
fi

cpu_file=$1

# Check if the file exists
if [ ! -f "$cpu_file" ]; then
    echo "Error: File $cpu_file not found."
    exit 1
fi

# Read file contents and calculate sum
sum=0
count=0

while IFS= read -r line; do
    sum=$(awk "BEGIN {printf \"%.6f\", $sum + $line}")
    count=$((count + 1))
done < "$cpu_file"

# Calculate average
if [ "$count" -eq 0 ]; then
    echo "Error: No numbers found in the file."
    exit 1
fi

average=$(awk "BEGIN {printf \"%.6f\", $sum / $count}")

echo "Sum: $sum"
echo "Average: $average"
