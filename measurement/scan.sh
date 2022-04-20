#!/bin/bash

source remote.sh
source parameters.sh

# Generate scripts for measurements
for ((i=$client_thread_lower_bound;i<=$client_thread_upper_bound;i++))
do
    for ((j=$store_lower_bound;j<=$store_upper_bound;j++))
    do
        thread=$((2 ** i))
        store=$((4 ** j))
        msg_num=$((total_request/thread))
        echo "creating script for $thread threads per client and $store store size..."
        directory=pb-$server_semantics_code-$client_semantics_code-$thread-$store
        mkdir $directory
        cd $directory

        # usage for primary: ./server 1 sematics_code thread_num store_size self_addr self_port backup_addr backup_port ...
        echo "ssh $PRIMARY ./primary_backup_server 1 $server_semantics_code $server_thread $store $PRIMARY_ADDR $PRIMARY_PORT $BACKUP1_ADDR $BACKUP1_PORT $BACKUP2_ADDR $BACKUP2_PORT &" > script.sh

        # usage for backup: ./server 0 sematics_code thread_num store_size self_addr self_port backup_id
        echo "ssh $BACKUP1 ./primary_backup_server 0 $server_semantics_code $server_thread $store $BACKUP1_ADDR $BACKUP1_PORT 0 &" >> script.sh
        echo "ssh $BACKUP2 ./primary_backup_server 0 $server_semantics_code $server_thread $store $BACKUP2_ADDR $BACKUP2_PORT 1 &" >> script.sh

        # Wait for servers spinning up
        echo "sleep 5" >> script.sh

        # usage: ./client semantics_code thread_num msg_num store_size file_name_prefix self_addr self_port primary_addr primary_port backup_addr backup_port ...
        echo "ssh $CLIENT1 ./primary_backup_client $client_semantics_code $thread $msg_num $store $directory $CLIENT1_ADDR $CLIENT1_PORT $PRIMARY_ADDR $PRIMARY_PORT $BACKUP1_ADDR $BACKUP1_PORT $BACKUP2_ADDR $BACKUP2_PORT" >> script.sh

        chmod +x script.sh
        cd ../
    done
done

# Run measurements
for ((i=$client_thread_lower_bound;i<=$client_thread_upper_bound;i++))
do
    for ((j=$store_lower_bound;j<=$store_upper_bound;j++))
    do
        thread=$((2 ** i))
        store=$((4 ** j))
        directory=pb-$server_semantics_code-$client_semantics_code-$thread-$store
        cd $directory

        echo "Running measurements for $thread threads per client and $store store size..."
        start=$SECONDS
        ./script.sh
        end=$SECONDS
        echo "Ended in $((end - start)) seconds"
        echo $((end - start)) > running_time.txt

        echo "Stoping servers..."
        ssh $PRIMARY "ps -ef | grep primary_backup_server | grep -v grep | awk '{print \$2}' | xargs kill"
        ssh $BACKUP1 "ps -ef | grep primary_backup_server | grep -v grep | awk '{print \$2}' | xargs kill"
        ssh $BACKUP2 "ps -ef | grep primary_backup_server | grep -v grep | awk '{print \$2}' | xargs kill"

        echo "Moving results into their directory..."
        ssh $CLIENT1 "mkdir $directory"
        ssh $CLIENT1 "mv ${directory}_* $directory/"

        cd ../
    done
done