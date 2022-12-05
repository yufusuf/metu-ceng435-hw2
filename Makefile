all:
	gcc -g sock.c server.c -o server -lpthread
	gcc -g sock.c client.c -o client -lpthread
clean:
	rm -rf server client
