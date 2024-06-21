server: server.c headers.h
	gcc -o server server.c;

s: server
	./server

client: client.c headers.h
	gcc -o client client.c;

c: client
	./client

clean:
	rm -rf client server