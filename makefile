CC=gcc
CFLAGS=-g

main:
	$(CC) -o server server.c $(CFLAGS) -lpthread
	$(CC) -o client client.c $(CFLAGS)

clean:
	rm server
	rm client