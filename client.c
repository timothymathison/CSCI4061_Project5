// CSci 4061 Spring 2017 Assignment 5
// Name1=Timothy Mathison
// Name2=Clay O'Neil
// StudentID1=ID mathi464
// StudentID2=ID oneil512

#include <string.h>
#include <stdlib.h>
#include <regex.h> 
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
	//char * directory_path = (char *)malloc(1024);
	char * config_name = (char *)malloc(256);
	FILE * config;
	char * server_ip = (char *)malloc(32);
	char * port = (char *)malloc(8);
	char * chunk_size = (char *)malloc(8);

	//check number of arguments
	if(argc != 2)
	{
		printf("Incomplete command arguments: 2 are required %d where provided\n", argc);
		printf("Usage: ./client client.config\n");
		exit(1);
	}

	strcpy(config_name, argv[1]);

	config = fopen(config_name, "r");
	if(config == NULL)
	{
		perror("Client config can't be found");
		exit(1);
	}

	printf("PID: %ld\n", (long int)getpid());

	struct stat* stat_info = malloc(1024);
	stat(config_name, stat_info);
	char * config_info = (char *)malloc(stat_info->st_size);
	fread(config_info, 1, stat_info->st_size, config);
	char * line = (char *)malloc(1031);

	regex_t re_server;
	regex_t re_port;
	regex_t re_chunk_size;
	char r1[] = "^Server = *";
	char r2[] = "^Port = *";
	char r3[] = "^Chunk_Size = *";
	regcomp(&re_server, r1, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_port, r2, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_chunk_size, r3, REG_EXTENDED|REG_ICASE|REG_NOSUB);

	//Read config file
	int port_found = 0;
	int server_found = 0;
	int chunk_size_found = 0;
	int line_index = 0;
	int i;
	for(i = 0; i < stat_info->st_size; i++)
	{
		if(config_info[i] != 13 && config_info[i] != 10 && config_info[i] != 0)
		{
			line[line_index++] = config_info[i];
			if(!(i == stat_info->st_size - 1))
			{
				continue;
			}
		}

		if(!port_found && regexec(&re_port, line, 0, NULL,0) == 0)
		{
			strcpy(port, &line[6]);
			port_found = 1;
		}
		else if(!server_found && regexec(&re_server, line, 0, NULL,0) == 0)
		{
			strcpy(server_ip, &line[8]);
			server_found = 1;
		}
		else if(!chunk_size_found && regexec(&re_chunk_size, line, 0, NULL,0) == 0)
		{
			strcpy(chunk_size, &line[12]);
			chunk_size_found = 1;
		}
		if(port_found && server_found && chunk_size_found)
		{
			break;
		}
		line_index = 0;
		bzero((char *)line, 1031);
	}
	if(!server_found)
	{
		printf("No server found in config file %s\n", config_name);
		exit(1);
	}
	if(!port_found)
	{
		printf("No port found in config file %s\n", config_name);
		exit(1);
	}
	if(!chunk_size_found)
	{
		printf("No Chunk_Size found in config file %s\n", config_name);
		exit(1);
	}

	//remove leading spaces from ip address
	while(server_ip[0] == ' ')
	{
		server_ip = &server_ip[1];
	}
	printf("Server: %s\n", server_ip);
	printf("Port: %s\n", port);
	printf("Chunk_Size: %s\n",chunk_size);
	
	//Create socket
	int soc = socket(AF_INET, SOCK_STREAM, 0);
	if(soc < 0)
	{
		perror("Failed to open socket");
		exit(1);
	}

	struct sockaddr_in server_addr;
	struct hostent * server;
	int port_num = atoi(port);
	if(port_num == 0)
	{
		printf("Failed to parse port number");
		exit(1);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	int parse_ip = inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
	if(parse_ip <= 0)
	{
		printf("Failed to parse IP address: %s\n", server_ip);("Failed to parse IP address");
		int n;
		for(n = 0; n < 12; n++)
		{
			printf("%d\n", server_ip[n]);
		}
		exit(1);
	}

	printf("Conecting with server IP: %d\n", server_addr.sin_addr.s_addr);
	if(connect(soc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Failed to connect to the server");
		exit(1);
	}
	printf("Conection established with server \n");

	char buffer[8];
	bzero(buffer,8);
	strcpy(buffer, "Hi");
	int sent = write(soc, buffer, strlen(buffer));
	if(sent < 0)
	{
		perror("Failed to send message");
		exit(1);
	}

	int recieve = read(soc, buffer, 7);
	if(recieve < 0)
	{
		perror("Failed to recieve message");
		exit(1);
	}
	printf("Message Recieved: %s\n", buffer);

	//TODO: start requesting and processing data from the server

	close(soc);
}