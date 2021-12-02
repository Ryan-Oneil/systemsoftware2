CC=gcc
CFLAGS=-g

main:
	$(CC) -o server server.c $(CFLAGS)
	$(CC) -o client client.c $(CFLAGS)

clean:
	rm server
	rm client