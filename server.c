// webserver.c
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#define PORT 8080
#define BUFFER_SIZE 1024*16


int writeall(int fd, const char *resp, size_t len)
{
	size_t sent = 0;
	do
	{
		int res = write(fd, resp + sent, len - sent);
		if( res >= 0)
			sent += res;
		else
			return res;
	}
	while(sent < len);

	return sent;
}

int readheaders(int fd, char *buf, size_t len, int *headerend)
{
	size_t received = 0;
	do
	{
		int res = read(fd, &buf[received], len - received);
		if( res >= 0)
		{
			char *headend;
			int he1 = INT_MAX, he2 = INT_MAX;
			buf[received + res] = 0;
			headend = strstr(buf, "\n\n");
			if(headend)
				he1 = headend - buf + 2;
			headend = strstr(buf, "\r\n\r\n");
			if(headend)
				he2 = headend - buf + 4;

			received += res;
			if(he2 < he1) he1  =he2;
			if(he1 != INT_MAX)
			{
				*headerend = he1;
				return received;
			}
		}
		else
			return res;
	}
	while(received < len);

	return -1;
}


void create_directories(const char *path)
{
	const char *dir_begin = path, *dir_begin_next;
	char dir_path[PATH_MAX] = "";
	size_t dir_len = 0;
	while(dir_begin_next = strchr(dir_begin, '/'))
	{
		dir_begin_next++;
		memcpy(&dir_path[0] + (dir_begin - path), dir_begin, dir_begin_next - dir_begin);
		printf("mkdir %s\n",dir_path);
		mkdir(dir_path, 0777);
		dir_begin = dir_begin_next;
	}
}

#define MAX_RESP_SIZE 32768
void serve_file(const char *path, char *buffer, int newsockfd, const char *mime )
{
	char resp_head[256] = "";
	char resp[MAX_RESP_SIZE] = "";
	int fd = open(path, O_RDONLY);
	int resp_len = snprintf(resp_head, 255,
							"HTTP/1.0 200 OK\r\n"
							"Server: webserver-c\r\n"
							"Content-type: %s\r\n\r\n", mime);

	if( fd < 0 ) return;
	read(fd, resp, MAX_RESP_SIZE);
	close(fd);

	// Write to the socket
	writeall(newsockfd, resp_head, resp_len);
	int valwrite = writeall(newsockfd, resp, strlen(resp));
	if (valwrite < 0) {
		perror("webserver (write)");
		return;
	}
}


void serve_list(const char *path, char *buffer, int fd)
{
	char resp_dir[MAX_RESP_SIZE];
	char resp_fil[MAX_RESP_SIZE];
	char fpath[PATH_MAX] = {};
	int len_dir = 0, len_fil = 0;
	int plen = strlen(path);
	if(!plen)
	{
		path = ".";
		plen = 1;
	}
	const char resp_list[] =
		"HTTP/1.0 200 OK\r\n"
		"Server: webserver-c\r\n"
		"Content-type: application/json\r\n\r\n[\n";
	DIR *dirp;
	writeall(fd, resp_list, sizeof(resp_list) - 1);
	printf("list %s\n", path);
	dirp = opendir(path);
	if (!dirp)
		return;
	if( plen > PATH_MAX - 2)
		plen = PATH_MAX - 2;
	strncpy(fpath, path, plen);
	fpath[plen++] = '/';


	while (1) {
		struct dirent *dp;
		struct stat sb;

		dp = readdir(dirp);
		if (!dp)
			break;
		if (dp->d_name[0] == '.')// && !dp->d_name[1])
			continue;
		strncpy(&fpath[plen], dp->d_name, PATH_MAX - plen - 1);
		if (stat(fpath, &sb) != 0)
			continue;
		printf("dir %s\n", dp->d_name);
		if(S_ISDIR(sb.st_mode))
			len_dir += snprintf(&resp_dir[0] + len_dir, MAX_RESP_SIZE - len_dir, "{\"name\": \"%s\", \"type\": 0, \"size\": %d},\n", dp->d_name, (int)sb.st_size);
		else
			len_fil += snprintf(&resp_fil[0] + len_fil, MAX_RESP_SIZE - len_fil, "{\"name\": \"%s\", \"type\": 1, \"size\": %d},\n", dp->d_name, (int)sb.st_size);
	}
	closedir(dirp);
	writeall(fd, resp_dir, len_dir);
	writeall(fd, resp_fil, len_fil);
	const char end[] = "{\"name\": \"\", \"type\": -1, \"size\": 0}]";
	writeall(fd, end, sizeof(end) - 1);
}

int main() {
	char buffer[BUFFER_SIZE];

	// Create a socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("webserver (socket)");
		return 1;
	}
	printf("socket created successfully\n");
	const int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");
	// Create the address to bind the socket to
	struct sockaddr_in host_addr;
	int host_addrlen = sizeof(host_addr);

	host_addr.sin_family = AF_INET;
	/*
	int fd = open("output", O_CREAT | O_WRONLY, 0744);
	write(fd, buffer + he, valread - he);
	while((valread = read( newsockfd, buffer, BUFFER_SIZE )) >= 0)
		write(fd, buffer, valread);
	close(fd);*/
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Create client address
	struct sockaddr_in client_addr;
	int client_addrlen = sizeof(client_addr);

	// Bind the socket to the address
	if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
		perror("webserver (bind)");
		return 1;
	}
	printf("socket successfully bound to address\n");

	// Listen for incoming connections
	if (listen(sockfd, SOMAXCONN) != 0) {
		perror("webserver (listen)");
		return 1;
	}
	printf("server listening for connections\n");

	for (;;) {
		// Accept incoming connections
		int newsockfd = accept(sockfd, (struct sockaddr *)&host_addr,
							   (socklen_t *)&host_addrlen);
		if (newsockfd < 0) {
			perror("webserver (accept)");
			continue;
		}
		printf("connection accepted\n");

		// Get client address
		int sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr,
								(socklen_t *)&client_addrlen);
		if (sockn < 0) {
			perror("webserver (getsockname)");
			continue;
		}
		int he = -1;

		// Read from the socket
		int valread = readheaders(newsockfd, buffer, BUFFER_SIZE - 1, &he);
		if (valread < 0) {
			perror("webserver (read)");
			continue;
		}

		// Read the request
		char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];
		const char *contentlength;
		int clen = 0;
		if((contentlength = strcasestr(buffer, "content-length: ")))
		{
			clen = atoi(contentlength + sizeof("content-length: ") - 1);
		}
		sscanf(buffer, "%s %s %s", method, uri, version);
		printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
			   ntohs(client_addr.sin_port), method, version, uri);


		if(!strcmp(method,"PUT"))
		{
			int r = fork();
			size_t filelen = 0;
			if(r == 0) // child, copy the file
			{
				//handleupload(newsockfd, buffer, uri);
				int fd;
				char *path = uri;
				if(strncmp(path, "/data/", 6) || strstr(path, ".."))
					_exit(1);

				path += 6;
				while(path[0] == '/')path++;
				create_directories(path);
				fd = open(path, O_CREAT | O_WRONLY, 0666);
				write(fd, buffer + he, valread - he);
				filelen = valread - he;
				printf("transfer0 %d\n",(int)valread);
				while( filelen < clen )
				{
					int readlen = clen - filelen;
					if( readlen > BUFFER_SIZE )
						readlen = BUFFER_SIZE;
					valread = read( newsockfd, buffer, readlen );
					if(valread > 0)
					{
						filelen += valread;
						printf("transfer %d\n",(int)valread);
						write(fd, buffer, valread);
					}
					else break;
				}
				printf("done %s\n", path);
				close(fd);
				const char resp_ok[] = "HTTP/1.0 200 OK\r\n"
								   "Server: webserver-c\r\n"
								   "Content-type: text/html\r\n\r\n"
								   "OK";
				if( valread >= 0)
					writeall(newsockfd, resp_ok, sizeof(resp_ok) - 1);
				close(newsockfd);
				_exit(0);
			}
			else
			{
				close(newsockfd);
				if(r < 0)
				{
					perror("fork");
					close(sockfd);
					return 1;
				}
			}
		}
		else if(!strcmp(method, "DELETE"))
		{
			char *path = uri;
			if(strstr(path, ".."))
			{
				close(newsockfd);
				continue;
			}
			if(!strncmp(path, "/data/", 6))
			{
				path += 6;
				unlink(path);
				const char resp_ok[] = "HTTP/1.0 200 OK\r\n"
									   "Server: webserver-c\r\n"
									   "Content-type: text/html\r\n\r\n"
									   "OK";
				writeall(newsockfd, resp_ok, sizeof(resp_ok) - 1);
			}
			close(newsockfd);
		}
		else
		{
			char *path = uri;
			if(strstr(path, ".."))
			{
				close(newsockfd);
				continue;
			}
			if(!strncmp(path, "/list/", 6))
			{
				path += 6;
				serve_list(path, buffer, newsockfd);
			}
			else if(strncmp(path, "/data/", 6))
			{
				serve_file("folderupload.html", buffer, newsockfd, "text/html");
			}
			else
			{
				path += 6;
				serve_file(path, buffer, newsockfd, "text/plain");
			}

			close(newsockfd);
		}
	}

	close(sockfd);
	return 0;
}
