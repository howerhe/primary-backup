#!/bin/bash

source parameters.sh

# Throughput unit: /(sec * thread)
echo "thread, store, throughput" > throughput_summary.csv
echo "thread, store, violation" > violation_summary.csv

for ((i=$client_thread_lower_bound;i<=$client_thread_upper_bound;i++))
do
    for ((j=$store_lower_bound;j<=$store_upper_bound;j++))
    do
        thread=$((2 ** i))
        store=$((8 ** j))
        directory=pb-$server_semantics_code-$client_semantics_code-$thread-$store
        RScript individual.R $directory $directory $thread
        cd $directory
        elapsed=`cat client_time.txt`
        violations=`cat violations.txt`
        throughput=$((1000 * total_request / thread / elapsed))
        cd ../
        echo "$thread, $store, $throughput" >> throughput_summary.csv
        echo "$thread, $store, $violations" >> violation_summary.csv
    done
done

RScript violation.R