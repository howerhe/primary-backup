#!/bin/bash

source parameters.sh
echo ""

# Throughput unit: /(sec * thread)
echo "# violations in primary" >primary_error_summary.txt
echo "thread, store, throughput" >throughput_summary.csv
echo "thread, store, violation" >violation_summary.csv

current_measurement=0
total_measurements=$(((client_thread_upper_bound + 1) * $store_upper_bound))
for ((i = $client_thread_lower_bound; i <= $client_thread_upper_bound; i++)); do
    for ((j = $store_lower_bound; j <= $store_upper_bound; j++)); do
        current_measurement=$((current_measurement + 1))
        echo "(measurement $current_measurement/$total_measurements)"
        thread=$((2 ** i))
        store=$((8 ** j))
        directory=pb-$server_semantics_code-$client_semantics_code-$thread-$store
        RScript individual.R $directory $directory $client_semantics_code $thread $store
        cd $directory
        primary_errors=$(cat primary_errors.txt)
        elapsed=$(cat client_time.txt)
        violations=$(cat violations.txt)
        throughput=$((1000 * total_request / thread / elapsed))
        cd ../
        echo "$primary_errors" >>primary_error_summary.txt
        echo "$thread, $store, $throughput" >>throughput_summary.csv
        echo "$thread, $store, $violations" >>violation_summary.csv
    done
done
echo ""

RScript violation.R