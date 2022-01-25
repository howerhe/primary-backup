measurement: server client

server: server.c src/store.c src/message.h src/socket.c src/synchronization.c src/pool.c src/consistency.c
	gcc -Wall -ggdb -o server -std=gnu99 src/store.c src/socket.c src/synchronization.c src/pool.c src/consistency.c server.c -lpthread
client: client.c src/message.h src/socket.c
	gcc -Wall -ggdb -o client -std=gnu99 src/socket.c zipf/c_avlt.c zipf/zipf.c client.c -lpthread -lm

store_test:
	gcc -Wall -ggdb -o store_test -std=gnu99 src/store.c test/store.c -lpthread
socket_test:
	gcc -Wall -ggdb -o socket_test -std=gnu99 src/socket.c test/socket.c
synchronization_test:
	gcc -Wall -ggdb -o synchronization_test -std=gnu99 src/synchronization.c test/synchronization.c -lpthread
pool_demo:
	gcc -Wall -ggdb -o pool_demo -std=gnu99 src/pool.c test/pool_demo.c -lpthread