all:
	gcc -g sock.c server.c -o server -lpthread -Wall -pedantic 
	gcc -g sock.c client.c -o client -lpthread -Wall -pedantic 
clean:
	rm -rf server client
