# Primary-Backup

Code and measurement scripts for different semantics of primary-backup servers. It is used to measure semantic violations when servers and clients are using different consistency semantics.

Run `setup.sh` to start.

## Code

The code is written in C, to simplify the verification process. All the code for primary-backup is inside `code`.

`server.c` is the driver for the primary server and backup servers. `client.c` is the driver for the client for measurement.

Inside `src` are modules for primary-backup. Their unit tests are in `test`.

Code for Zipf's distribution is provided by Dr. Ji-Yong Shin, and is in `zipf`.

## Design

### Server

When a server starts, there is a main thread listening to requests from clients in a loop, and a thread pool with indicated number of threads to handle incoming requests.

The life cycle of a message from clients or a synchronization message from the primary is like following. A request is received by the main thread and handled by a thread from the thread pool with corresponding handling functions. And the life for this message ends.

For backups, if the message is a synchronization request from the primary, it will synchronize its own store, and then send back acknowledgement to the primary. If the message is a read request from a client, it will read from its own store, and wait for finishing synchronization at the index (from other threads) (depends on consistency semantics used and version number), then it will send response back to clients.

If it's a write request to the primary, the store of the primary is updated first, and then synchronization to backup servers starts. Firstly, the handling thread handling will send synchronization messages to backups. Then the thread enters a timed conditional wait. There is a data structure to store synchronization acknowledgement messages for each working thread. Synchronization acknowledgement messages are kept in a message queue and it will alarm the working thread in the primary to check whether the synchronizations has finished. The thread can also wake up after a timeout. If there are backups that haven't finished synchronization, synchronization requests will be sent to those backups again.

### Client

The client program is used for running measurement experiments. The number of threads and the number of requests are adjustable.

For each single `client_routine`, it starts with a random number with Zipf's distribution to choose the index of the block storage to operate. And then there is another random number with even distribution to decide whether to read or write, and a third random number with even distribution to decide which backup to read if it's a read operation (always read from backup servers), or the new number to write. The time spent for an operation is measured. All the relating information is kept in a log and saved into a file after finishing all the operations.

### Potential Improvement

The client would hang when the number of request is large, and when the servers are using monotonic read and clients are using read-my-writes.

"primary violation" occurs in some experiments.

## Measurement

The measurement running and analysis scripts are inside `measurement`.

Before running measurements, remote servers should be setup, scripts should be made executable, and `server` should be renamed to `primary_backup_server`, `client` to `primary_backup_client` and both of them are moved to the home directory.

`remote.sh` is for connecting remote servers. `parameters.sh` is for setting parameters for measurements. Those are utility scripts and will be called by other scripts.

`scan.sh` can generate scripts for a series of measurements and run the measurements. `retrieve.sh` is used to retrieve data from remote servers for analysis. It will also clean up files generated on remote servers. `kill.sh` is used to kill server and client processes on remote servers.

When running `analyze.sh`, it will call `individual.R` to plot version difference and latency versus round, and `violation.R` to plot violations of consistency under different combinations of client thread numbers and store sizes.

So for a typical experiment and measurement, modify parameters in `parameters.sh` first. Then run `scan.sh`. After experiments on remote servers finish, run `retrieve.sh`. If the program hang, run `kill.sh` and do a manual cleaning on remote servers. After finishing retrieval, run `analyze.sh` to get measurement.
