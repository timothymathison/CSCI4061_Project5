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
//#include <pthread.h>
//#include <time.h>
//#include <sys/time.h>
//#include <signal.h>
//#include <libgen.h>

int search_directory( char * buffer[], int offset, int buffer_length, char dir_name[])

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

	config = fopen(config_name);
	if(config == NULL)
	{
		perror("Server config can't be found");
	}

	char * line = (char *)malloc(1031);
	while(fgets(line, 1032, config) != NULL)
	{
		
	}

}

int search_directory( char * buffer[], int offset, int buffer_length, char dir_name[])
{
	DIR* directory;
	struct dirent *file;
	char * name;
	char * path;
	int loc = offset;

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
            loc += search_directory(buffer, loc, path);
		}
		else
		{
			if(loc >= buffer_length) //buffer is not long enough
			{
				return -1;
			}
			path = (char *)malloc(1280);
			strcpy(path, dir_name);
			strcat(path, "/");
			strcat(path, file->d_name);
			
			buffer[loc] = path;

			loc++;
		}
	}
	closedir(directory);
	return loc - offset;
}