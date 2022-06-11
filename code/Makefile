measurement: server client

server: server.c src/store.c src/message.c src/socket.c src/pool.c src/consistency.c
	gcc -Wall -ggdb -o server -std=gnu11 src/store.c src/message.c src/socket.c src/pool.c src/consistency.c server.c -lpthread
client: client.c src/message.c src/socket.c
	gcc -Wall -ggdb -o client -std=gnu11 src/message.c src/socket.c zipf/c_avlt.c zipf/zipf.c client.c -lpthread -lm

store_test: src/store.c test/store.c
	gcc -Wall -ggdb -o store_test -std=gnu11 src/store.c test/store.c -lpthread
message_test: src/message.c test/message.c
	gcc -Wall -ggdb -o message_test -std=gnu11 src/message.c test/message.c -lpthread
socket_test: src/socket.c test/socket.c
	gcc -Wall -ggdb -o socket_test -std=gnu11 src/socket.c test/socket.c
pool_demo: src/pool.c test/pool_demo.c
	gcc -Wall -ggdb -o pool_demo -std=gnu11 src/pool.c test/pool_demo.c -lpthread