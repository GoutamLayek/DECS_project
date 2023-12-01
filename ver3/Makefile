CC=gcc

all: server submit

server: server.o queue.o
	gcc server.o queue.o -o server -lpthread

server.o: server.c
	gcc -c -g server.c

queue.o: queue.c queue.h
	gcc -c queue.c

submit: client.c
	gcc client.c -o submit

clean:
	rm -rf *.o submit server
