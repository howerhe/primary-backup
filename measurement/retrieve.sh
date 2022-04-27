#!/bin/bash

source remote.sh
source parameters.sh

for ((i=$client_thread_lower_bound;i<=$client_thread_upper_bound;i++))
do
    for ((j=$store_lower_bound;j<=$store_upper_bound;j++))
    do
        thread=$((2 ** i))
        store=$((8 ** j))
        directory=pb-$server_semantics_code-$client_semantics_code-$thread-$store
        cd $directory

        echo "retrieving results for $thread threads per client and $store blocks store..."
        scp khoury:~/$directory/${directory}_*.csv ./
        scp khoury:~/$directory/client_time.txt ./

        echo "cleaning results for $thread threads per client and $store blocks store..."
        ssh $CLIENT1 "rm -rf $directory"

        cd ../
    done
done