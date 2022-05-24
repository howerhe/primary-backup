#!/bin/bash

source remote.sh
echo ""
source parameters.sh
echo ""

current_measurement=0
total_measurements=$(((client_thread_upper_bound + 1) * $store_upper_bound))
for ((i=$client_thread_lower_bound;i<=$client_thread_upper_bound;i++))
do
    for ((j=$store_lower_bound;j<=$store_upper_bound;j++))
    do
        current_measurement=$((current_measurement + 1))
        echo "(measurement $current_measurement/$total_measurements)"
        thread=$((2 ** i))
        store=$((8 ** j))
        directory=pb-$server_semantics_code-$client_semantics_code-$thread-$store
        cd $directory

        echo "retrieving results for $thread threads per client and $store blocks store..."
        scp khoury:~/$directory/* ./

        echo "cleaning results for $thread threads per client and $store blocks store..."
        ssh $CLIENT1 "rm -rf $directory"

        cd ../
        echo ""
    done
done