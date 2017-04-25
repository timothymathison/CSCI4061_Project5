// CSci 4061 Spring 2017 Assignment 4
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
//#include <pthread.h>
//#include <time.h>
//#include <sys/time.h>
//#include <signal.h>
//#include <libgen.h>

int search_directory( char * buffer[], int offset, int buffer_length, char dir_name[]);

int main(int argc, char *argv[])
{
	char * directory_path = (char *)malloc(1024);
	char * config_name = (char *)malloc(256);
	FILE * config;
	char * port = (char *)malloc(8);

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

	for (i = 0; i < count; i++)
	{
		stat(image_paths[i], stat_info);
		fprintf(catalog, "%s, %ld\n", image_paths[i], stat_info->st_size);
		//TODO: create checksum
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
	printf("%d\n", port_num);
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

	//TODO: set socket address
	//TODO: wait for connection from client
	//inet_aton()  convert ip string to binary 
}

int search_directory( char * buffer[], int offset, int buffer_length, char dir_name[])
{
	DIR* directory;
	struct dirent *file;
	char * name;
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