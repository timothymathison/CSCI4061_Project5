// CSci 4061 Spring 2017 Assignment 5
// Name1=Timothy Mathison
// Name2=Clay O'Neil
// StudentID1=ID mathi464
// StudentID2=ID oneil512

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <regex.h> 
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
//#include <pthread.h>
//#include <time.h>
//#include <sys/time.h>
//#include <signal.h>
#include <libgen.h>

#include "md5sum.h"

int search_directory( char * buffer[], int offset, int buffer_length, char dir_name[]);
int min(int a, int b);
char * get_address(char * line);

int main(int argc, char *argv[])
{
	char * directory_path = (char *)malloc(1024);
	char * config_name = (char *)malloc(256);
	FILE * config;
	char * port = (char *)malloc(8);
	char * buffer1;
	char * file_buffer;
	char * fline = (char *)malloc(2048);
	char * address = (char *)malloc(1024);
	char * buffer2; 
	int k = 0;
	int c_size;
	int w = 0;

	//check number of arguments
	if(argc != 2)
	{
		printf("Incomplete command arguments: 2 are required %d where provided\n", argc);
		printf("Usage: ./server server.config\n");
		exit(1);
	}

	strcpy(config_name, argv[1]);

	config = fopen(config_name, "r");
	if(config == NULL)
	{
		perror("Server config can't be found");
		exit(1);
	}

	printf("PID: %ld\n", (long int)getpid());

	struct stat* stat_info = malloc(1024);
	stat(config_name, stat_info);
	char * config_info = (char *)malloc(stat_info->st_size);
	fread(config_info, 1, stat_info->st_size, config);
	char * line = (char *)malloc(1031);

	regex_t re_port;
	regex_t re_dir;
	char r1[] = "^Port = *";
	char r2[] = "^Dir = *";
	regcomp(&re_port, r1, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_dir, r2, REG_EXTENDED|REG_ICASE|REG_NOSUB);

	//Read config file
	int port_found = 0;
	int dir_found = 0;
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
		else if(!dir_found && regexec(&re_dir, line, 0, NULL,0) == 0)
		{
			strcpy(directory_path, &line[5]);
			dir_found = 1;
		}
		if(port_found && dir_found)
		{
			break;
		}
		line_index = 0;
		bzero((char *)line, 1031);
	}
	if(!port_found)
	{
		printf("No port found in config file %s\n", config_name);
		exit(1);
	}
	if(!dir_found)
	{
		printf("No directory found in config file %s\n", config_name);
		exit(1);
	}

	while(port[0] == ' ')
	{
		port = &port[1];
	}
	printf("Port: %s\n", port);

	//Process directory path
	char * temp = (char *)malloc(1024);
	while(directory_path[0] == ' ')
	{
		directory_path = &directory_path[1];
	}
	if(directory_path[0] == '.' && directory_path[1] == '/')
	{
		getcwd(temp, 1024);
		strcat(temp, &directory_path[1]);
		strcpy(directory_path, temp);
	}
	else if(directory_path[0] != '/')
	{
		strcpy(temp, "/");
		strcat(temp, directory_path);
		strcpy(directory_path, temp);
	}
	printf("Directory: %s\n", directory_path);

	//check if directory exists
	DIR* directory;
	directory = opendir(directory_path);
	if(directory ==  NULL)
	{
		perror("Directory could not be found");
		exit(1);
	}
	closedir(directory);

	//create list of image files in directory path
	int buffer_length = 256;
	char * image_paths[buffer_length];
	int count = 0;
	count = search_directory(image_paths, 0, buffer_length, directory_path);
	while(count == -1)
	{
		buffer_length = 2 * buffer_length;
		char * image_paths[buffer_length]; //this doesn't work need to find another way to resize
		count = search_directory(image_paths, 0, buffer_length, directory_path);
	}

	for(i = 0; i < count; i++)
	{
		printf("Image found: %s\n", image_paths[i]);
	}

	//create catalog csv file
	char * catalog_path = (char *)malloc(1036);
	strcpy(catalog_path, directory_path);
	strcat(catalog_path, "/catalog.csv");
	FILE * catalog;
	catalog = fopen(catalog_path, "w");
	if(catalog == NULL)
	{
		perror("Failed to create catalog file");
		exit(1);
	}
	fputs("Filename, Size, Checksum\n", catalog);
	unsigned char sum[MD5_DIGEST_LENGTH];
	int check_i;
	for (i = 0; i < count; i++)
	{
		stat(image_paths[i], stat_info);
		md5sum(image_paths[i], sum);
		fprintf(catalog, "%s, %ld,", basename(image_paths[i]), stat_info->st_size);
		for(check_i = 0; check_i < MD5_DIGEST_LENGTH; check_i++)
		{
			fprintf(catalog, "%02x", sum[i]);
		}
		fputs("\n", catalog);		
	}
	fclose(catalog);

	//Create socket
	int soc = socket(AF_INET, SOCK_STREAM, 0);
	if(soc < 0)
	{
		perror("Failed to open socket");
		exit(1);
	}
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	bzero((char *) &server_addr, sizeof(server_addr));
	int port_num = atoi(port);
	if(port_num == 0)
	{
		printf("Failed to parse port number");
		exit(1);
	}
	//printf("%d\n", port_num);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port_num);
	int result = bind(soc, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if(result < 0)
	{
		perror("Failed to bind socket and address");
		exit(1);
	}
	listen(soc, 5);
	int client_length = sizeof(client_addr);
	int newsoc = accept(soc, (struct sockaddr *)&client_addr, (socklen_t *)&client_length);
	if(newsoc < 0)
	{
		perror("Error with incoming message");
		exit(1);
	}

	char buffer[3000];
	bzero(buffer, 3000);
	int recieved = read(newsoc, buffer, 7);
	if(recieved < 0)
	{
		perror("Error reading message");
		exit(1);
	}
	printf("Message: %s\n", buffer);
	bzero(buffer, 3000);
	strcpy(buffer, "Here!");
	int sent = write(newsoc, buffer, strlen(buffer));
	if(sent < 0)
	{
		perror("Failed to send message");
	}

	// Get buffer request
	bzero(buffer, 3000);
	recieved = read(newsoc, buffer, 3000);
	if(recieved == 0)
	{
		perror("Error reading message");
		exit(1);
	}
	
	if(!strcmp("catalog.csv",buffer))
	{
		FILE *fp = fopen(catalog_path, "r");
		if (fp != NULL)
		{
			fseek (fp, 0, SEEK_END);
			int length = ftell(fp);
			printf("%d length\n", length);
			fseek (fp, 0, SEEK_SET);
			buffer1 = (char *)calloc(length, 1);
			if (buffer1)
			{
				fread (buffer1, 1, length, fp);
			}
			fclose(fp);
		}
		int sent = write(newsoc, buffer1, strlen(buffer1));
			
	}
	//Get buffer size
	bzero(buffer, 3000);
	strcpy(buffer, "chunk_size");
	sent = write(newsoc, buffer, strlen(buffer));
	recieved = read(newsoc, buffer, 3000);
	c_size = atoi(buffer);
	//`printf("%d chunk size is \n", c_size);
	file_buffer = (char *)calloc(c_size, 1);

	//Read input of first files to download
	FILE *fp = fopen(catalog_path, "r");
	while(1)
	{
		bzero(file_buffer, c_size);
		recieved = read(newsoc, file_buffer, c_size);

		if(!strcmp(file_buffer, "0"))
		{
			break;
		} 

		//Get file name
		int num = atoi(file_buffer) - 1;
		address = image_paths[num]; 

		// Send file
		FILE *f = fopen(address, "rb");
		if (f != NULL)
		{
			fseek (f, 0, SEEK_END);
			int len_f = ftell(f);
			printf("%d len of file \n", len_f);
			fseek (f, 0, SEEK_SET);
			buffer2 = (char *)calloc(len_f, 1);

			fread (buffer2, 1, len_f, f);
			fclose(f);

			int total = len_f;
			int amount = min(total, c_size);
			int top = ceil(len_f / (double)c_size);

			for(k = 0; k < top; k++)
			{
				bzero(file_buffer, c_size);
				amount = min(total, c_size);

				for(w = 0; w < amount; w++)
				{
					buffer[w] = buffer2[(k * c_size) + w];
				}

				//printf("amount is %d\n", amount);
				sent = write(newsoc, buffer, amount);
				//printf("sent is %d\n", sent);
				total -= amount;
				//printf("total left is %d\n", total);
			}
		}
	}
	fclose(fp);

	close(soc);

	return 0;
}

int search_directory( char * buffer[], int offset, int buffer_length, char dir_name[])
{
	DIR* directory;
	struct dirent *file;
	//char * name;
	char * path;
	int loc = offset;
	int sub_count;
	regex_t re_image;
	char re[] = "^.*png$|^.*jpg$|^.*gif$|^.*tiff$";
	regcomp(&re_image, re, REG_EXTENDED|REG_ICASE|REG_NOSUB);

	directory = opendir(dir_name);
	while((file = readdir(directory)) != NULL)
	{
		if(file->d_type == 4)//file is a subdirectory
		{
			if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0)
			{
                continue;
			}
			path = (char *)malloc(1280);
			strcpy(path, dir_name);
            strcat(path, "/");
            strcat(path, file->d_name);
            sub_count = search_directory(buffer, loc, buffer_length, path);
            if(sub_count == -1)
            {
            	return -1;
            }
            else
            {
            	loc += sub_count;
            }
		}
		else
		{
			if(loc >= buffer_length) //buffer is not long enough
			{
				return -1;
			}
			if(regexec(&re_image, file->d_name, 0, NULL,0) == 0)
			{
				path = (char *)malloc(1280);
				strcpy(path, dir_name);
				strcat(path, "/");
				strcat(path, file->d_name);
				buffer[loc] = path;
				loc++;
			}
			
		}
	}
	closedir(directory);
	return loc - offset;
}

int min(int a, int b)
{
	return (a < b)? a : b;
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
