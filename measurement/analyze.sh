#!/bin/bash

source parameters.sh

echo "thread, store, violation" > violation_summary.csv

for ((i=$client_thread_lower_bound;i<=$client_thread_upper_bound;i++))
do
    for ((j=$store_lower_bound;j<=$store_upper_bound;j++))
    do
        thread=$((2 ** i))
        store=$((4 ** j))
        directory=pb-$server_semantics_code-$client_semantics_code-$thread-$store
        RScript individual.R $directory $directory $thread
        cd $directory
        violations=`cat violations.txt`
        cd ../
        echo "$thread, $store, $violations" >> violation_summary.csv
    done
done

RScript violation.R