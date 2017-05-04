.PHONY: all clean

all: image_manager_server image_manager_client

image_manager_server: server.c md5sum.o
	cc -Wall -o server md5sum.o server.c -lm -lcrypto

image_manager_client: client.c md5sum.o
	cc -Wall -o client md5sum.o client.c -lcrypto

md5sum.o: md5sum.c
	cc -Wall -c -o md5sum.o md5sum.c

clean:
	rm -f *.o

