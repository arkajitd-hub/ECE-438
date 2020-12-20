/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <arpa/inet.h>

#define PORT "3490"		   // the port client will be connecting to
#define DEFAULT_PORT "80"  // the default port if not specified
#define MAXDATASIZE 131072 // max number of bytes we can get at once (should be 16 megabytes)

int header_handler(int sockfd, char *host, char *port, char *path);
int server_header_handler(char *buffer, char *status_code, int n, FILE *output);
int extract_info(char *host, char *port, char *path, char *arg);
// int write_output(int sockfd, char *buff, FILE *fd);

/**
 * Creates and sends the client header to the server.
 * @param sockfd	socket file descriptor
 * @param host      hostname of connected server
 * @param port      port of connected server
 * @param path      path of requested file
 */
int header_handler(int sockfd, char *host, char *port, char *path)
{
	// User-Agent: Wget/1.12 (linux-gnu)\r\n
	char header_buff[4096];
	sprintf(header_buff, "GET %s HTTP/1.1\r\nUser-Agent: Wget/1.12 (linux-gnu)\r\nHost: %s:%s\r\nConnection: Keep-Alive\r\n\r\n", path, host, port);
	if (send(sockfd, header_buff, strlen(header_buff), 0) == -1)
	{
		return -1;
	}
	return 0;
}

/**
 * Takes the header from the server and breaks it apart
 * Also finds where the header ends and adjusts the buffer accordingly.
 * 
 * @param buffer		from the sockeet
 * @param status_code	either 200 404 or 400
 */
int server_header_handler(char *buffer, char *status_code, int n, FILE *output)
{
	// HTTP/1.X 200 OK
	int bf = 0;
	int sc = 0;
	char *begin_file;

	while (buffer[bf] != ' ')
	{
		bf++;
	}
	bf++;

	//takes the status code from the header file.
	for (sc = 0; sc < 3; sc++)
	{
		status_code[sc] = buffer[bf];
		bf++;
	}

	//if we dont get 200 as the status code.
	if (strcmp(status_code, "200") != 0)
	{
		return -1;
	}

	begin_file = strstr(buffer, "\r\n\r\n");
	begin_file += 4;

	//change the amount of bytes
	n -= begin_file - buffer;
	if (fwrite(begin_file, 1, n, output) != n)
	{
		return -1;
	}

	return 0;
}

/**
 * From the argument, extras the host, port and file path of the request.
 * The host, port, and path parameters hold the parts from the argument.
 *
 * NOTE: need to allocate host port and path buffers in memory using memset
 *
 * @param arg		argument 
 * @param host      hostname of connected server
 * @param port      port of connected server
 * @param path      path of requested file
 */
int extract_info(char *host, char *port, char *path, char *arg)
{
	int i, h, k, p, flag_80;
	char http[7];
	//stores the first 7 bytes into http to check if the http header is there.
	for (i = 0; i < 7; i++)
	{
		http[i] = arg[i];
	}
	//if the argument does not start with http://, fail
	if (strcmp(http, "http://") != 0)
	{
		return -1;
	}

	//increment characters
	i = 7;
	h = 0;
	k = 0;
	p = 0;
	flag_80 = 0;

	//stroes the host part of the argument
	while (arg[i] != '/' && arg[i] != ':')
	{
		host[h] = arg[i];
		h++;
		i++;
	}

	//stores the port of the argument if it exists
	if (arg[i] == '/')
	{
		sprintf(port, "80");
		flag_80 = 1;
	}
	else if (arg[i] == ':')
	{
		i++;
		while (arg[i] != '/')
		{
			port[p] = arg[i];
			i++;
			p++;
		}
	}

	// stores the path part of the argument
	while (arg[i] != '\0')
	{
		path[k] = arg[i];
		k++;
		i++;
	}

	//add end of line don't know if we need it.
	host[h] = '\0';
	if (!flag_80)
	{
		port[p] = '\0';
	}
	path[k] = '\0';
	return 0;
}

// /**
//  * Gets file from server side and copies the file to output
//  * @param sockfd	socket file descriptor
//  * @param buff      do we need this?
//  */
// int write_output(int sockfd, char *buff, FILE *fd)
// {
// 	return 0;
// }

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{

	//this is for the argument.
	char path[512];
	char port[16];
	char host[512];
	memset(path, 0, sizeof path);
	memset(port, 0, sizeof port);
	memset(host, 0, sizeof host);

	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2)
	{
		fprintf(stderr, "usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (extract_info(host, port, path, argv[1]) == -1)
	{
		fprintf(stderr, "does not start with http://\n");
	}
	// printf("host: %s, port: %s, path: %s", host, port, path);

	if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			  s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	// Send the header file to the server side
	if (header_handler(sockfd, host, port, path) == -1)
	{
		printf("sending file failed");
	}

	// AFTER DONE WITH CONNECTION

	FILE *output_file = fopen("output", "wb");
	int header_flag = 1;
	char status[3];

	while (1)
	{
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		if (numbytes == 0)
		{
			printf("done");
			break;
		}

		if (header_flag)
		{
			if (server_header_handler(buf, status, numbytes, output_file))
			{
				fprintf(stderr, "status code not equal to 200\n");
				exit(1);
			}
			header_flag = 0;
		}
		else
		{
			if (fwrite(buf, 1, numbytes, output_file) != numbytes)
			{
				fprintf(stderr, "error in transfering from buffer to output file\n");
				exit(1);
			}
		}
	}

	fclose(output_file);

	// buf[numbytes] = '\0';

	// printf("client: received '%s'\n", buf);

	close(sockfd);

	return 0;
}
