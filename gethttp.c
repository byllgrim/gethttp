/* See LICENSE file */
#include <sys/socket.h>
#include <netdb.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* function declarations */
static void die(char *fmt, ...);
static void initsock(void);
static void handleconn(int fd);
static void respond(int fd, char *path);

/* global variables */
static int sockfd;
static char *service = "http-alt";
static char *dir = NULL;
static char defaultdoc[] = "/index.html";
static char getresponse[] = "HTTP/1.0 200 OK\nContent-Type: text/html\n\n";

/* function definitions */
void
die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(EXIT_FAILURE);
}

void
initsock(void)
{
	struct addrinfo hints;
	struct addrinfo *res;
	int err, one = 1;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_PASSIVE;
	if ((err = getaddrinfo(NULL, service, &hints, &res))) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(err));
		exit(EXIT_FAILURE);
	}

	sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	if (sockfd < 0)
		die("socket: %s", strerror(errno));

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)))
		die("setsockopt: %s", strerror(errno));

	if (bind(sockfd, res->ai_addr, res->ai_addrlen))
		die("bind: %s", strerror(errno));

	if (listen(sockfd, SOMAXCONN))
		die("listen: %s", strerror(errno));

	freeaddrinfo(res);
}

void
handleconn(int fd)
{
	char buf[BUFSIZ+1];

	bzero(buf, sizeof(buf));
	read(fd, buf, BUFSIZ);
	//printf("received: %s", buf);

	if (!strcmp(strtok(buf, " "), "GET"))
		respond(fd, strtok(NULL, " "));
}

void
respond(int fd, char *path)
{
	char buf[BUFSIZ];
	char *file;
	char *msg;
	int tmpfd;

	file = calloc(BUFSIZ, sizeof(char));
	strncat(file, dir, BUFSIZ/3);
	strncat(file, path, BUFSIZ/3);
	strncat(file, defaultdoc, BUFSIZ/3);
	if ((tmpfd = open(file, O_RDONLY)) < 0)
		die("open: %s\n", strerror(errno));

	bzero(buf, sizeof(buf));
	read(tmpfd, buf, sizeof(buf) - 1);

	msg = calloc(sizeof(getresponse) + sizeof(buf), sizeof(char));
	strncat(msg, getresponse, sizeof(getresponse));
	strncat(msg, buf, sizeof(buf));
	send(fd, msg, strlen(msg), 0);
}

int
main(int argc, char *argv[1])
{
	int tmpfd;
	pid_t pid;

	initsock();
	dir = getenv("HOME");

	for (;;) {
		if ((tmpfd = accept(sockfd, 0, 0)) < 0) {
			perror("");
			continue;
		}

		switch (pid = fork()) {
		case -1:
			die("fork: %s", strerror(errno));
		case 0:
			close(sockfd);
			handleconn(tmpfd);
			close(tmpfd);
			exit(EXIT_SUCCESS);
		default:
			close(tmpfd);
		}
	}

	return EXIT_SUCCESS;
}
