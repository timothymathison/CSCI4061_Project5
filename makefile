image_manager_server: server.c
	gcc -o server server.c

image_manager_client: client.c
	gcc -o client client.c

clean:
	rm -f server
	rm -f client