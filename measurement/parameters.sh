#!/bin/bash

# Semantics code for server and client
# eventual = 0, read-my-writes = 1, monotonic-reads = 2
export server_semantics_code=0
export client_semantics_code=0

echo "server semantics code: $server_semantics_code"
echo "client semantics code: $client_semantics_code"

# Total requests from client: 2^13 = 131072
# total_request = thread * msg_num
export total_request=131072

echo "total requests: $total_request"

# Storage size: 4^2 to 4^7
export store_lower_bound=2
export store_upper_bound=7

echo "storage size: 4^$store_lower_bound to 4^$store_upper_bound"

# Threads per server: 50
export server_thread=50

echo "threads per server: $server_thread"

# Threads per client: 2^0 to 2^7
export client_thread_lower_bound=0
export client_thread_upper_bound=7

echo "threads per client: 2^$client_thread_lower_bound to 2^$client_thread_upper_bound"