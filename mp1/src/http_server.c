/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490" // the port users will be connecting to

#define BACKLOG 10 // how many pending connections queue will hold
#define HEADER "HTTP/1.1 200 OK \r\n\r\n"
#define ERR_404 "HTTP/1.1 404 FILE NOT FOUND \r\n\r\n"
#define ERR_400 "HTTP/1.1 400 ERROR \r\n\r\n"
#define MAX_PORT 65535
#define MAXDATASIZE 1000000
char buffer[1024];
void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}
void log_state(char *buff)
{
	FILE *file = fopen("log.txt", "a+");
	fputs(buff, file);
	fclose(file);
}

void extraction(char *httpmsg, char *command)
{
	char *start_req;
	char *end_req;
	start_req = httpmsg;
	end_req = httpmsg;

	while (*start_req != '/')
	{
		start_req++;
	}
	start_req++;
	end_req = strchr(start_req, ' ');

	int l = strlen(start_req) - strlen(end_req);

	strncpy(command, start_req, l); // command extracts the file name from HTTP message.
}

int get_message(const char *filename, char **message)
{

	FILE *file = NULL;
	int file_len = 0;

	if (filename == NULL)
	{
		return 0;
	}

	file = fopen(filename, "rb");
	if (file == NULL)
	{
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "file open failure: %s\n", strerror(errno));
		log_state(buffer);
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	file_len = ftell(file);
	rewind(file);

	*message = (char *)malloc(file_len);
	fread(*message, file_len, 1, file);
	fclose(file);

	return file_len;
}

int http_response(const char *command, char **message)
{

	char *file_buffer = NULL;
	char header_buffer[1024];
	int file_length;

	file_length = get_message(command, &file_buffer);

	if (file_length == -1 || file_length == 0)
	{
		sprintf(header_buffer, ERR_404);
	}

	memset(header_buffer, 0, sizeof header_buffer);
	sprintf(header_buffer, HEADER);
	int head_len = strlen(header_buffer);

	int total_len = head_len + file_length;

	*message = (char *)malloc(total_len);

	char *tmp = *message;
	memcpy(tmp, header_buffer, head_len);
	memcpy(&tmp[head_len], file_buffer, file_length);

	if (file_buffer)
	{
		free(file_buffer);
	}

	return total_len;
}

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

	int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if (argc != 2)
	{
		fprintf(stderr, "usage: server portnumber\n");
		exit(1);
	}

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
					   sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while (1)
	{ // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1)
		{
			perror("accept");
			continue;
		}

		char buffer[1024];
		char command[1024];
		memset(buffer, 0, sizeof(buffer));
		memset(command, 0, sizeof(command));
		printf("check%d\n", 3);
		rv = recv(new_fd, buffer, sizeof(buffer), 0);
		printf("rv:%d", rv);
		if (rv == 0)
		{
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "Error: No messages received");
		}

		inet_ntop(their_addr.ss_family,
				  get_in_addr((struct sockaddr *)&their_addr),
				  s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork())
		{				   // this is the child process
			close(sockfd); // child doesn't need the listener
			extraction(buffer, command);
			char message[MAXDATASIZE];
			FILE *file;
			int sent = 0;
			int n;
			int firstRead = 1;
			file = fopen(command, "rb");
			memset(message, '\0', sizeof(message));
			sprintf(message, HEADER);

			while (1)
			{
				if (firstRead)
				{
					n = fread(message + strlen(HEADER), sizeof(char), MAXDATASIZE - strlen(HEADER), file);
					n = send(new_fd, message, n + strlen(HEADER), 0);
					if (n == -1)
					{
						perror("Error:");
						break;
					}
					firstRead = 0;
				}
				else
				{
					n = fread(message, sizeof(char), MAXDATASIZE, file);
					n = send(new_fd, message, n, 0);
					if (n == -1)
					{
						perror("Error:");
						break;
					}
				}
				if (n == 0)
				{
					break;
				}

				sent += n;
			}

			close(new_fd);

			exit(0);
		}
		close(new_fd); // parent doesn't need this
	}

	return 0;
}
