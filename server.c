// webserver.c
// strcasestr
#define _GNU_SOURCE
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
void serve_file(const char *path, char *buffer, int newsockfd, const char *mime, int binary)
{
	char resp_head[256] = "";
	char resp[MAX_RESP_SIZE] = "";
	int fd = open(path, O_RDONLY);
	int resp_len = snprintf(resp_head, 255,
							"HTTP/1.1 200 OK\r\n"
							"Server: webserver-c\r\n"
							"Content-type: %s\r\n\r\n", mime);

	if( fd < 0 ) return;

	writeall(newsockfd, resp_head, resp_len);

	while(( resp_len = read(fd, resp, MAX_RESP_SIZE)) > 0)
	{
		int valwrite = writeall(newsockfd, resp, resp_len);
		// Write to the socket
		if (valwrite < 0) {
			perror("webserver (write)");
			return;
		close(fd);
		}
	}
	close(fd);
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
		"HTTP/1.1200 OK\r\n"
		"Server: webserver-c\r\n"
		"Content-Type: application/json\r\n\r\n[\n";
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

void serve_list_dav(const char *path, char *buffer, int fd)
{
	char resp[MAX_RESP_SIZE];
	char fpath[PATH_MAX] = {};
	int len_dir = 0, len_fil = 0;
	int plen = strlen(path);
	if(!plen)
	{
		path = ".";
		plen = 1;
	}
	const char resp_list[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: webserver-c\r\n"
		"Content-Type: text-xml\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\" ?><D:multistatus xmlns:D=\"DAV:\">";
	DIR *dirp;
	writeall(fd, resp_list, sizeof(resp_list) - 1);
	printf("list %s\n", path);
	dirp = opendir(path);
	if (!dirp)
		return;
	if( plen > PATH_MAX - 2)
		plen = PATH_MAX - 2;
	strncpy(fpath, path, plen);
	/*len_dir += snprintf(
		&resp_dir[0] + len_dir, MAX_RESP_SIZE - len_dir,
		"<D:response><D:href>/files/%s</D:href><D:propstat><D:prop>"
		"<D:creationdate>Wed, 30 Oct 2019 18:58:08 GMT</D:creationdate>"
		"<D:getetag>\"01572461888\"</D:getetag>"
		"<D:getlastmodified>Wed, 30 Oct 2019 18:58:08 GMT</D:getlastmodified>"
		"<D:resourcetype><D:collection/></D:resourcetype>"
		"</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response></D:multistatus>",
		fpath);*/
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
		printf("dir %s %d %d\n", dp->d_name, len_fil, len_dir);

		if(S_ISDIR(sb.st_mode))
		{
			int len = snprintf(resp, MAX_RESP_SIZE - 1,
				"<D:response><D:href>/files/%s</D:href><D:propstat><D:prop>"
				"<D:creationdate>Wed, 30 Oct 2019 18:58:08 GMT</D:creationdate>"
				"<D:getetag>\"01572461888\"</D:getetag>"
				"<D:getlastmodified>Wed, 30 Oct 2019 18:58:08 GMT</D:getlastmodified>"
				"<D:resourcetype><D:collection/></D:resourcetype>"
				"<d:quota-used-bytes>163</d:quota-used-bytes><d:quota-available-bytes>11802275840</d:quota-available-bytes>"
				"</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>",
				dp->d_name);
			writeall(fd, resp, len);
		}
		else
		{
			int len = snprintf(
				resp, MAX_RESP_SIZE - 1,
				"<D:response><D:href>/files/%s</D:href><D:propstat><D:prop>"
				"<D:creationdate>Wed, 30 Oct 2019 18:58:08 GMT</D:creationdate>"
				"<D:getcontentlength>%d</D:getcontentlength>"
				"<D:getetag>\"01572461888\"</D:getetag>"
				"<D:getlastmodified>Wed, 30 Oct 2019 18:58:08 GMT</D:getlastmodified>"
				"<D:resourcetype /><d:getcontenttype>text/plain</d:getcontenttype>"
				"</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>",
				dp->d_name, (int)sb.st_size);
			writeall(fd, resp, len);
		}

	}
	closedir(dirp);
	const char end[] = "</D:multistatus>";
	writeall(fd, end, sizeof(end) - 1);
}

void serve_path_dav(const char *path, char *buffer, int fd)
{
	char resp_dir[MAX_RESP_SIZE];
	int len_dir = 0, len_fil = 0;
	int plen = strlen(path);
	struct stat sb;

	if(!plen)
	{
		path = ".";
		plen = 1;
	}
	int s = stat(path, &sb);
	if(s || S_ISDIR(sb.st_mode) && sb.st_size == 0)
	{
		const char resp_list1[] =
			"HTTP/1.1 207 OK\r\n"
			"Server: webserver-c\r\n"
			"Content-Type: text-xml\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\" ?><D:multistatus xmlns:D=\"DAV:\" />";
		writeall(fd, resp_list1, sizeof(resp_list1) - 1);
		printf("bad stat\n");
		return;

	}
	const char resp_list[] =
		"HTTP/1.1 200 OK\r\n"
		"Server: webserver-c\r\n"
		"Content-Type: text-xml\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\" ?><D:multistatus xmlns:D=\"DAV:\">";
	writeall(fd, resp_list, sizeof(resp_list) - 1);


	len_dir += snprintf(
		&resp_dir[0] + len_dir, MAX_RESP_SIZE - len_dir,
		"<D:response><D:href>/files/%s</D:href><D:propstat><D:prop>"
		"<D:creationdate>Wed, 30 Oct 2019 18:58:08 GMT</D:creationdate>"
		"<D:getetag>\"01572461888\"</D:getetag>"
		"<D:getlastmodified>Wed, 30 Oct 2019 18:58:08 GMT</D:getlastmodified>"
		"%s"
		"</D:prop><D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response></D:multistatus>",
		path, S_ISDIR(sb.st_mode)?"<d:quota-used-bytes>163</d:quota-used-bytes><d:quota-available-bytes>11802275840</d:quota-available-bytes><D:resourcetype><D:collection/></D:resourcetype>":"<D:resourcetype /><d:getcontenttype>text/plain</d:getcontenttype>");

	writeall(fd, resp_dir, len_dir);
}


int main() {
	char buffer[BUFFER_SIZE] = { };

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
			close(newsockfd);
			continue;
		}
		int he = -1;

		// Read from the socket
		int valread = readheaders(newsockfd, buffer, BUFFER_SIZE - 1, &he);
		if (valread < 0) {
			perror("webserver (read)");
			close(newsockfd);
			continue;
		}

		// Read the request
		char method[32] = "", uri[256] = "", version[32] = "";
		const char *contentlength = strcasestr(buffer, "content-length: ");
		int clen = 0;
		if(contentlength)
		{
			clen = atoi(contentlength + sizeof("content-length: ") - 1);
		}
		if( sscanf(buffer, "%31s %255s %31s", method, uri, version) != 3 )
		{
			printf("Bad headers!\n");
			close(newsockfd);
			continue;
		}

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
				if(strncmp(path, "/files/", 7) || strstr(path, ".."))
					_exit(1);

				path += 7;
				while(path[0] == '/')path++;
				create_directories(path);
				fd = open(path, O_CREAT | O_WRONLY, 0666);
				write(fd, buffer + he, valread - he);
				filelen = valread - he;
				//printf("transfer0 %d\n",(int)valread);
				while( filelen < clen )
				{
					int readlen = clen - filelen;
					if( readlen > BUFFER_SIZE )
						readlen = BUFFER_SIZE;
					valread = read( newsockfd, buffer, readlen );
					if(valread > 0)
					{
						filelen += valread;
						//printf("transfer %d\n",(int)valread);
						write(fd, buffer, valread);
					}
					else break;
				}
				//printf("done %s\n", path);
				ftruncate(fd,filelen);
				close(fd);
				const char resp_ok_beg[] = "HTTP/1.1 201 Created\r\n"
								   "Server: webserver-c\r\n"
								   "Location: /files/";
				const char resp_ok_end[] = "\r\nContent-type: text/html\r\n\r\n"
								   "OK";

				if( valread >= 0)
				{
					writeall(newsockfd, resp_ok_beg, sizeof(resp_ok_beg) - 1);
					writeall(newsockfd, path, strlen(path));
					writeall(newsockfd, resp_ok_end, sizeof(resp_ok_end) - 1);
				}
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
			if(!strncmp(path, "/files/", 7))
			{
				path += 7;
				unlink(path);
				const char resp_ok[] = "HTTP/1.1 200 OK\r\n"
									   "Server: webserver-c\r\n"
									   "Content-type: text/html\r\n\r\n"
									   "OK";
				writeall(newsockfd, resp_ok, sizeof(resp_ok) - 1);
			}
			close(newsockfd);
		}
		else if(!strcmp(method, "GET"))
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
			else if(strncmp(path, "/files/", 7))
			{
				serve_file("folderupload.html", buffer, newsockfd, "text/html", 1);
			}
			else
			{
				path += 7;
				serve_file(path, buffer, newsockfd, "text/plain", 1);
			}

			close(newsockfd);
		}
		else if(!strcmp(method, "MKCOL"))
		{
			char *path = uri;
			const char resp_ok[] = "HTTP/1.1 201 Created\r\n"
								   "Server: webserver-c\r\n"
								   "Access-Control-Allow-Origin: *\r\n"
								   "Content-type: text/html\r\n\r\n"
								   "OK";
			if(strncmp(path, "/files/", 7) || strstr(path, ".."))
			{
				close(newsockfd);
				continue;
			}
			path += 7;
			create_directories(path);
			mkdir(path, 0777);

			if( valread >= 0)
				writeall(newsockfd, resp_ok, sizeof(resp_ok) - 1);
			close(newsockfd);
		}
		else if(!strcmp(method, "MOVE"))
		{
			char *path = uri;
			const char resp_ok[] = "HTTP/1.1 201 Created\r\n"
								   "Server: webserver-c\r\n"
								   "Content-type: text/html\r\n\r\n"
								   "OK";
			const char *dest;
			if(strncmp(path, "/files/", 7) || strstr(path, ".."))
			{
				close(newsockfd);
				continue;
			}
			path += 7;

			dest = strcasestr(buffer, "destination: ");
			if(dest)
			{
				char *e1 = strstr(dest, "\n");
				char *e2 = strstr(dest, "\r\n");
				if(e2 && e2 < e1)
					e1 = e2;
				*e1 = 0;
				dest += sizeof("destination: " ) - 1;
				printf("move %s %s\n", path, dest);
				if(!strncmp(dest, "/files/", 7) && !strstr(dest, ".."))
				{
					dest += 7;
					rename(path, dest);
				}
			}

			if( valread >= 0)
				writeall(newsockfd, resp_ok, sizeof(resp_ok) - 1);
			close(newsockfd);
		}
		else if(!strcmp(method, "PROPFIND"))
		{
			char *path = uri;
			const char resp_auth[] = "HTTP/1.1 401 Unauthorized\r\n"
								   "Server: webserver-c\r\n"
								   "WWW-Authenticate: Basic realm=\"User Visible Realm\"\r\n\r\n";
			int filelen = valread - he;
			//printf("transfer0 %d\n",(int)valread);
			while( filelen < clen )
			{
				int readlen = clen - filelen;
				if( readlen > BUFFER_SIZE )
					readlen = BUFFER_SIZE;
				valread = read( newsockfd, buffer, readlen );
				if(valread > 0)
				{
					filelen += valread;
					//printf("transfer %d\n",(int)valread);
					write(1, buffer, valread);
				}
				else break;
			}
			/*if(!strcasestr(buffer, "authorization: "))
			{
				writeall(newsockfd, resp_auth, sizeof(resp_auth) - 1);
				close(newsockfd);
			}*/
			if(strncmp(path, "/files/", 6) || strstr(path, ".."))
			{
				serve_path_dav(".", buffer, newsockfd);
				close(newsockfd);
				continue;
			}
			path += 7;
			printf("%s\n", buffer);
			if(strcasestr( buffer, "Depth: 0" ))
			{
				serve_path_dav(path, buffer, newsockfd);
			}
			else
			{
				serve_list_dav(path, buffer, newsockfd);
			}

			close(newsockfd);
		}
		else if(!strcmp(method, "OPTIONS"))
		{
			const char resp_ok[] = "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Access-Control-Allow-Methods: PROPFIND, PROPPATCH, COPY, MOVE, DELETE, MKCOL, PUT, GETLIB, VERSION-CONTROL, CHECKIN, CHECKOUT, UNCHECKOUT, REPORT, UPDATE, CANCELUPLOAD, HEAD, OPTIONS, GET, POST\r\n"
			"Access-Control-Allow-Headers: Overwrite, Destination, Content-Type, Depth, User-Agent, X-File-Size, X-Requested-With, If-Modified-Since, X-File-Name, Cache-Control\r\n"
			"Access-Control-Max-Age: 86400\r\n\r\n";
			if( valread >= 0)
				writeall(newsockfd, resp_ok, sizeof(resp_ok) - 1);
			close(newsockfd);
		}
		else if(!strcmp(method, "LOCK"))
		{
			const char resp_ok[] = "HTTP/1.1 405 Method not allowed\r\n"
								   "Content-Type: text/plain\r\n"
								   "Content-Length: 0\r\n";
								   /*"\r\n\r\n<?xml version=\"1.0\" encoding=\"utf-8\" ?><D:prop xmlns:D=\"DAV:\"> <D:lockdiscovery><D:activelock> <D:locktype><D:write/></D:locktype><D:lockscope><D:exclusive/></D:lockscope><D:depth>Infinity</D:depth>"
									"<D:owner></D:owner><D:timeout>Second-604800</D:timeout><D:locktoken><D:href>opaquelocktoken:e71d4fae-5dec-22d6-fea5-00a0c91e6be4</D:href></D:locktoken></D:activelock></D:lockdiscovery></D:prop>";*/
			if( valread >= 0)
				writeall(newsockfd, resp_ok, sizeof(resp_ok) - 1);
			close(newsockfd);
		}
		else
		{
			// todo: answer 4XX
			close(newsockfd);
		}
	}

	close(sockfd);
	return 0;
}
