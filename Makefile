all:
	gcc sock.c server.c -o server -lpthread
	gcc sock.c client.c -o client -lpthread
clean:
	rm -rf server client
