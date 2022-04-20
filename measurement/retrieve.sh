#!/bin/bash

source remote.sh
source parameters.sh

for ((i=$client_thread_lower_bound;i<=$client_thread_upper_bound;i++))
do
    for ((j=$store_lower_bound;j<=$store_upper_bound;j++))
    do
        thread=$((2 ** i))
        store=$((4 ** j))
        directory=pb-$server_semantics_code-$client_semantics_code-$thread-$store
        cd $directory

        echo "Retrieving results for $thread threads per client and $store store size..."
        scp khoury:~/$directory/${directory}_*.csv ./

        echo "Cleaning results for $thread threads per client and $store store size..."
        ssh $CLIENT1 "rm -rf $directory"

        cd ../
    done
done