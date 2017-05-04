// CSci 4061 Spring 2017 Assignment 5
// Name1=Timothy Mathison
// Name2=Clay O'Neil
// StudentID1=ID mathi464
// StudentID2=ID oneil512

#include <libgen.h>
#include <arpa/inet.h>
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
#include <sys/ioctl.h>

char * get_address(char * line);
int main(int argc, char *argv[])
{
	//char * directory_path = (char *)malloc(1024);
	char * config_name = (char *)malloc(256);
	FILE * config;
	char * server_ip = (char *)malloc(32);
	char * port = (char *)malloc(8);
	char * chunk_size = (char *)malloc(8);
	char * image_type = (char *)malloc(8);
	char * input = (char *)malloc(10);
	char * address = (char *)malloc(1024);
	char * fline = (char *)malloc(1024);
	char * base = (char *)malloc(256);

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
	regex_t re_image_type;
	char r1[] = "^Server = *";
	char r2[] = "^Port = *";
	char r3[] = "^Chunk_Size = *";
	char r4[] = "^ImageType = *";
	regcomp(&re_server, r1, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_port, r2, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_chunk_size, r3, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_image_type, r4, REG_EXTENDED|REG_ICASE|REG_NOSUB);

	//Read config file
	int port_found = 0;
	int server_found = 0;
	int chunk_size_found = 0;
	int image_type_found = 0;
	int line_index = 0;
	int i;
	int passive_mode = 0;
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
		else if(!image_type_found && regexec(&re_image_type, line, 0, NULL,0) == 0)
		{
			strcpy(image_type, &line[11]);
			image_type_found = 1;
		}
		if(port_found && server_found && chunk_size_found && image_type_found)
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
	int image_type_num = 0;
	if(image_type_found)
	{
		passive_mode = 1;

		while(image_type[0] == ' ')
		{
			image_type = &image_type[1];
		}
		if(image_type[3] == ' ')
		{
			image_type[3] = '\0';
		}
		else if(image_type[4] == ' ')
		{
			image_type[4] = '\0';
		}
		// regex_t re_png;
		// regex_t re_jpg;
		// regex_t re_gif;
		// regex_t re_tiff;
		char pattern_png[] = "png";
		char pattern_jpg[] = "jpg";
		char pattern_gif[] = "gif";
		char pattern_tiff[] = "tiff";
		// regcomp(&re_png, pattern_png, REG_EXTENDED|REG_ICASE|REG_NOSUB);
		// regcomp(&re_jpg, pattern_jpg, REG_EXTENDED|REG_ICASE|REG_NOSUB);
		// regcomp(&re_gif, pattern_gif, REG_EXTENDED|REG_ICASE|REG_NOSUB);
		// regcomp(&re_tiff, pattern_tiff, REG_EXTENDED|REG_ICASE|REG_NOSUB);

		if(strcmp(image_type, pattern_png) == 0)
		{
			image_type_num = 1;
		}
		else if(strcmp(image_type, pattern_jpg) == 0)
		{
			image_type_num = 2;
		}
		else if(strcmp(image_type, pattern_gif) == 0)
		{
			image_type_num = 3;
		}
		else if(strcmp(image_type, pattern_tiff) == 0)
		{
			image_type_num = 4;
		}
		else
		{
			printf("Image type %s found in config file is invalid\n", image_type);
			exit(1);
		}
	}
	else
	{
		printf("No image type found, continuing in passive mode\n");
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
		printf("Failed to parse IP address: %s\n", server_ip);
		int n;
		for(n = 0; n < 20; n++)
		{
			printf("%c\n", server_ip[n]);
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

	int c_size = atoi(chunk_size);
	printf("%d\n", c_size);
	char buffer[c_size];
	bzero(buffer,c_size);
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

	// Request catalog file
	bzero(buffer,500);
	strcpy(buffer, "catalog.csv");
	sent = write(soc, buffer, strlen(buffer));
	if(sent == 0)
	{
		perror("Failed to send message");
		exit(1);
	}

	char cat_buffer[5000];
	bzero(cat_buffer,5000);
	int len = read(soc, cat_buffer, 5000);

	if(len < 0)
	{
		perror("Failed to recieve message");
		exit(1);
	}
	printf("Catalog Recieved: %s\n", cat_buffer);
	FILE *f = fopen("catalog.csv", "w+");
	fputs(cat_buffer, f);
	fclose(f);
	

	if(passive_mode)
	{
		if(!strcmp(image_type, "jpg"))
		{

		}
		

	}
	else
	{
		printf("INTERACTIVE MODE\n");
		while(1)
		{
			printf("Enter 0 to quit of the line number of a file to download\n");
			scanf("%s", input);

			if(!strcmp(input, "0"))
			{
				sent = write(soc, input, strlen(input));
				break;
			} 
			//get name of file
			int num = atoi(input);
			int cnt = 0;
			f = fopen("catalog.csv", "r");
			while(fgets(fline, 1024, f))
			{
				if(cnt == num)
				{
					address = get_address(fline);
					printf("address is %s\n", address);
				}
				cnt += 1;
			}
			fclose(f);
			
			//Send file request and read back file, hardcoded 500 as chunk size
			sent = write(soc, input, strlen(input));
			base = basename(address);
			int keep_reading = 1;
			printf("%s base\n", base);
			f = fopen(base, "wb+");
			while(keep_reading)
			{
				bzero(buffer,500);
				len = read(soc, buffer, 500);
				printf("%d amount read\n", len);
				fputs(buffer, f);
				if(len < 500)
				{
					keep_reading = 0;
				}
			}
			fclose(f);
	
			
			
		}
	}

	close(soc);
}

char * get_address(char * line)
{
	char * r = (char *)calloc(1024, 1);
	int len = strlen(line);
	int i = 0;
	for(i = 0; i < len; i++)
	{
		if((line[i] == ','))
		{
			break;
		} 
		r[i] = line[i];
	}
	return r;
}
