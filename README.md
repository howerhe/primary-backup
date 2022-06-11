# Primary-Backup

Code and measurement scripts for different semantics of primary-backup servers.

Run `setup.sh` to start.

## Code

The code is written in C, to simplify the verification process. All the code for primary-backup is inside `code`.

`server.c` is the driver for the primary server and backup servers. `client.c` is the driver for the client for measurement.

Inside `src` are modules for primary-backup. Their unit tests are in `test`.

Code for Zipf's distribution is provided by Dr. Ji-Yong Shin, and is in `zipf`.

## Design

### Server

When the primary server starts, there is a main thread listening to requests from clients in a loop, a `synchronize_receiver` thread listening to response from backup servers in a loop, `synchronize_sender` and `synchronize_refresher` threads whose numbers are equal to the number of backup servers. The equal number of `sync_q` and `sent_q` are also initiated for synchronization messages and sent messages.

The life cycle of a message in primary server is like following. Firstly, a request from a client is received by the main thread and handled by a thread from a thread pool with `request_handler` routine. This routine can run functions corresponding to the semantics for the primary server.

If the request is a read request, the life should end after the function respond to the client.

It it's a write request, the store of the primary is updated first, and then synchronization to backup servers starts. For a single backup server, there is a specific `sync_q`. When a new synchronization task is enqueued, a signal can alarm the dedicated `synchronize_sender` to send this message to the specific backup server, and the dequeued task in enqueued into `sent_q`.

If the synchronization is successful at the backup, a response can be received by the `synchronize_receiver` thread, and the task is removed from the `sent_q`. The comparing and removal is conducted by a thread from the thread pool with `synchronize_handler` routine. There are situations that the synchronization fails or is too slow. The dedicated `synchronize_refresher` thread scans `sent_q` according to a timer. If there are messages that are older than a threshold, these messages are moved backup to `sync_q` and synchronizations are tried again.

Notice that to distinguish request source easier, the port for the clients and the port for the backup servers are not the same. Each backup server has its specific `synchronize_sender`, `synchronize_refresher`, `sync_q` and `sent_q`. `request_handler` and `synchronize_handler` share the same thread pool.

The logic for backup servers are much simpler. It only has a main thread, and a thread pool for `request_handler` threads.

### Client

The client program is used for running measurement experiments. The number of threads and the number of requests are adjustable.

For each single `client_routine`, it starts with a random number with Zipf's distribution to choose the index of the block storage to operate. And then there is another random number with even distribution to decide whether to read or write, and a third random number with even distribution to decide which backup to read if it's a read operation (always read from backup servers), or the new number to write. The time spent for an operation is measured. All the relating information is kept in a log and saved into a file after finishing all the operations.

### Potential Improvement

The client should not have big problems.

The queues and stores used in servers need further inspection. Locks are used. There might be situations where dead lock happens.

There are too many sockets for the primary server. There should be a better approach to distinguish message source, sequence, and to avoid congestions (e.g. timeout).

The logic for message handling is too complex and there are too many types of threads. This happened before thread pool was introduced. If a better approach is used to distinguish message source and avoid hanging, and thread are redesigned with the idea of thread pool, the whole structure can be simplified.

## Measurement

The measurement running and analysis scripts are inside `measurement`.

Before running measurements, remote servers should be setup, scripts should be made executable, and `server` should be renamed to `primary_backup_server`, `client` to `primary_backup_client` and both of them are moved to the home directory.

`remote.sh` is for connecting remote servers. `parameters.sh` is for setting parameters for measurements. Those are utility scripts and will be called by other scripts.

`scan.sh` can generate scripts for a series of measurements and run the measurements. `retrieve.sh` is used to retrieve data from remote servers for analysis. It will also clean up files generated on remote servers. `kill.sh` is used to kill server and client processes on remote servers.

When running `analyze.sh`, it will call `individual.R` to plot version difference and latency versus round, and `violation.R` to plot violations of consistency under different combinations of client thread numbers and store sizes.
