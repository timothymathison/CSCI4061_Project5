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
#include <unistd.h>

#include "md5sum.h"

char * get_address(char * line);
int read_checksum(char * line, unsigned char * sum);
int find_image(char * image, char * directory_path);

int main(int argc, char *argv[])
{
	//char * directory_path = (char *)malloc(1024);
	struct stat* s = malloc(1024);
	char * config_name = (char *)malloc(256);
	FILE * config;
	char * server_ip = (char *)malloc(32);
	char * port = (char *)malloc(32);
	char * chunk_size = (char *)malloc(32);
	char * image_type = (char *)malloc(32);
	char * input = (char *)malloc(10);
	char * address = (char *)malloc(1024);
	char * fline = (char *)malloc(1024);
	char * base = (char *)malloc(256);
	char * path = (char *)malloc(2048);

	if(stat("images", s) == -1)
	{
		mkdir("images", 0700);
	}

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

	//printf("PID: %ld\n", (long int)getpid());

	struct stat* stat_info = malloc(1024);
	stat(config_name, stat_info);
	char * config_info = (char *)malloc(stat_info->st_size);
	fread(config_info, 1, stat_info->st_size, config);
	char * line = (char *)malloc(1031);

	regex_t re_server;
	regex_t re_port;
	regex_t re_chunk_size;
	regex_t re_image_type;

	regex_t re_jpg;
	regex_t re_gif;
	regex_t re_tif;
	regex_t re_png;

	char r1[] = "^Server = *";
	char r2[] = "^Port = *";
	char r3[] = "^Chunk_Size = *";
	char r4[] = "^ImageType = *";

	char r5[] = "^.*jpg*$";
	char r6[] = "^.*gif*$";
	char r7[] = "^.*tiff*$";
	char r8[] = "^.*png*$";

	regcomp(&re_server, r1, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_port, r2, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_chunk_size, r3, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_image_type, r4, REG_EXTENDED|REG_ICASE|REG_NOSUB);

	regcomp(&re_jpg, r5, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_gif, r6, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_tif, r7, REG_EXTENDED|REG_ICASE|REG_NOSUB);
	regcomp(&re_png, r8, REG_EXTENDED|REG_ICASE|REG_NOSUB);

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

	char pattern_png[] = "png";
	char pattern_jpg[] = "jpg";
	char pattern_gif[] = "gif";
	char pattern_tiff[] = "tiff";

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
	//struct hostent * server;
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
	bzero(buffer,c_size);
	strcpy(buffer, "catalog.csv");
	sent = write(soc, buffer, strlen(buffer));

	char cat_buffer[5000];
	bzero(cat_buffer,5000);
	int len = read(soc, cat_buffer, 5000);

	printf("Catalog Recieved: %s\n", cat_buffer);
	FILE *f = fopen("catalog.csv", "w+");
	fputs(cat_buffer, f);
	fclose(f);

	bzero(buffer,c_size);
	len = read(soc, buffer, c_size);

	if(!strcmp(buffer, "chunk_size"))
	{
		bzero(buffer,c_size);
		strcpy(buffer, chunk_size);
		sent = write(soc, buffer, strlen(buffer));
	}

	int keep_reading = 1;
	FILE * catalog;

	if(passive_mode)
	{
		//JPG
		if(!strcmp(image_type, "jpg"))
		{
			//get name of file
			catalog = fopen("catalog.csv", "r");
			int cnt = 0;
			char val[8];

			while(fgets(fline, 1024, catalog))
			{
				
				address = get_address(fline);
				if(regexec(&re_jpg, address, 0, NULL,0) == 0)
				{
					sprintf(val, "%d", cnt);
					sent = write(soc, val, strlen(val));

					base = basename(address);
					getcwd(path, 256);
					strcat(path, "/images/");
					strcat(path, base);

					f = fopen(path, "wb+");
					keep_reading = 1;
					int chunks = 0;
					while(keep_reading)
					{
						chunks++;
						bzero(buffer,c_size);
						len = read(soc, buffer, c_size);
						fwrite(buffer, 1, len, f);
						if(len < c_size)
						{
							keep_reading = 0;
						}
					}
					fclose(f);
					printf("Downloaded %s in %d chunks\n", address, chunks);

				}
				cnt += 1;
			}
			fclose(catalog);

		}

		//TIF
		if(!strcmp(image_type, "tiff"))
		{
			//get name of file
			catalog = fopen("catalog.csv", "r");
			int cnt = 0;
			char val[8];

			while(fgets(fline, 1024, catalog))
			{
				address = get_address(fline);
				if(regexec(&re_tif, address, 0, NULL,0) == 0)
				{
					sprintf(val, "%d", cnt);
					address = get_address(fline);
					sent = write(soc, val, strlen(val));

					base = basename(address);
					getcwd(path, 256);
					strcat(path, "/images/");
					strcat(path, base);

					f = fopen(path, "wb+");
					keep_reading = 1;
					int chunks = 0;
					while(keep_reading)
					{
						chunks++;
						bzero(buffer,c_size);
						len = read(soc, buffer, c_size);
						fwrite(buffer, 1, len, f);
						//fputs(buffer, f);
						if(len < c_size)
						{
							keep_reading = 0;
						}
					}
					fclose(f);
					printf("Downloaded %s in %d chunks\n", address, chunks);

				}
				cnt += 1;
			}
			fclose(catalog);

		}

		//GIF
		if(!strcmp(image_type, "gif"))
		{
			//get name of file
			catalog = fopen("catalog.csv", "r");
			int cnt = 0;
			char val[8];

			while(fgets(fline, 1024, catalog))
			{
				address = get_address(fline);
				if(regexec(&re_gif, address, 0, NULL,0) == 0)
				{
					sprintf(val, "%d", cnt);
					address = get_address(fline);
					sent = write(soc, val, strlen(val));

					base = basename(address);
					getcwd(path, 256);
					strcat(path, "/images/");
					strcat(path, base);

					f = fopen(path, "wb+");
					keep_reading = 1;
					int chunks = 0;
					while(keep_reading)
					{
						chunks++;
						bzero(buffer,c_size);
						len = read(soc, buffer, c_size);
						fwrite(buffer, 1, len, f);
						//fputs(buffer, f);
						if(len < c_size)
						{
							keep_reading = 0;
						}
					}
					fclose(f);
					printf("Downloaded %s in %d chunks\n", address, chunks);

				}
				cnt += 1;
			}
			fclose(catalog);

		}
		
		//PNG
		if(!strcmp(image_type, "png"))
		{
			//get name of file
			catalog = fopen("catalog.csv", "r");
			int cnt = 0;
			char val[8];

			while(fgets(fline, 1024, catalog))
			{
				address = get_address(fline);
				if(regexec(&re_png, address, 0, NULL,0) == 0)
				{
					sprintf(val, "%d", cnt);
					address = get_address(fline);
					sent = write(soc, val, strlen(val));

					base = basename(address);
					getcwd(path, 256);
					strcat(path, "/images/");
					strcat(path, base);

					f = fopen(path, "wb+");

					keep_reading = 1;
					int chunks = 0;
					while(keep_reading)
					{
						chunks++;
						bzero(buffer,c_size);
						len = read(soc, buffer, c_size);
						fwrite(buffer, 1, len, f);
						//fputs(buffer, f);
						if(len < c_size)
						{
							keep_reading = 0;
						}
					}
					fclose(f);
					printf("Downloaded %s in %d chunks\n", address, chunks);

				}
				cnt += 1;
			}
			fclose(catalog);

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
			catalog = fopen("catalog.csv", "r");
			while(fgets(fline, 1024, catalog))
			{
				if(cnt == num)
				{
					address = get_address(fline);
				}
				cnt += 1;
			}
			fclose(catalog);

			base = basename(address);
			getcwd(path, 256);
			strcat(path, "/images/");
			strcat(path, base);
			
			sent = write(soc, input, strlen(input));
			keep_reading = 1;
			f = fopen(path, "wb+");
			keep_reading = 1;
			int tot = 0;
			int chunks = 0;
			while(keep_reading)
			{
				chunks++;
				bzero(buffer,c_size);
				len = read(soc, buffer, c_size);
				tot += len;
				//printf("read is %d\n", len);
				fwrite(buffer, 1, len, f);
				if(len < c_size)
				{
					keep_reading = 0;
				}
			}
			fclose(f);
			printf("Downloaded %s in %d chunks\n", address, chunks);
		}
	}

	close(soc);

	printf("Creating HTML...\n");
	unsigned char sum[MD5_DIGEST_LENGTH];
	unsigned char calculated_sum[MD5_DIGEST_LENGTH];
	char * image_path = (char *)malloc(1024);
	char * image_name;
	FILE * html;
	html = fopen("download.html", "w");
	fputs("<html>\n", html);
	fputs("<head>\n", html);
	fputs("<title>Downloaded Images</title>\n", html);
	fputs("</head>", html);
	fputs("<body>\n", html);
	fputs("<h1>Downloaded Images</h1>", html);
	
	catalog = fopen("catalog.csv", "r");
	if(catalog == NULL)
	{
		perror("Could not open catalog.csv file");
		exit(1);
	}
	bzero(fline, 1024);
	fgets(fline, 1024, catalog); //skip first line
	while(fgets(fline, 1024, catalog))
	{
		strcpy(image_path, "./images/");
		image_name = get_address(fline);
		if(find_image(image_name, "images"))
		{
			strcat(image_path, image_name);
			int result = read_checksum(fline, sum);
			if(!result)
			{
				md5sum(image_path, calculated_sum);
				int check_i = 0;
				printf("----------------------------");
				for(check_i = 0; check_i < MD5_DIGEST_LENGTH; check_i++)
				{
					printf("%02x", sum[i]);
					printf("%02x\n", calculated_sum[i]);
				}
				if(memcmp(calculated_sum, sum, MD5_DIGEST_LENGTH) == 0)
				{
					fprintf(html, "<pre>(Checksum match!)    <a href='Image Link goes here'>%s</pre>\n", image_name);
				}
				else
				{
					fprintf(html, "<pre>(Checksum mismatch!)    %s</pre>\n", image_name);
				}	
			}
			else
			{
				fprintf(html, "<pre>(No Checksum in Catalog.csv)    %s</pre>\n", image_name);
			}
		}
		
		bzero(fline, 1024);
	}
	fclose(catalog);

	fputs("</body>\n", html);
	fputs("</html>\n", html);
	fclose(html);
	printf("HTML Created\n");

	return 0;
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

int read_checksum(char * line, unsigned char * sum)
{
	int comma_count = 0;
	int i = 0;
	int len = strlen(line);
	//char * sum = (char *)malloc(MD5_DIGEST_LENGTH);
	while(comma_count < 2)
	{
		if(i >= len)
		{
			return 1;
		}
		if(line[i] == ',')
		{
			comma_count++;
		}
		i++;
	}
	int check_i;
	for(check_i = i; check_i < MD5_DIGEST_LENGTH; check_i++)
	{
		sum[check_i] = line[i + check_i];
	}
	return 0;
}

int find_image(char * image, char * directory_path)
{
	DIR* directory;
	struct dirent *file;

	directory = opendir(directory_path);
	if(directory != NULL)
	{
		while((file = readdir(directory)) != NULL)
		{
			if(strcmp(file->d_name, image) == 0)
			{
				closedir(directory);
				return 1;
			}
		}
		closedir(directory);
		return 0;
	}
	perror("Images directory doesn't exist");
	exit(1);
}
