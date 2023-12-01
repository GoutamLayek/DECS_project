all: server submit

server: server.c
	gcc -o server server.c

submit: client.c
	gcc -o submit client.c

clean:
	rm -rf submit server

