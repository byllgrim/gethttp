/* See LICENSE file */
#include <sys/socket.h>
#include <netdb.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* function declarations */
static void die(char *fmt, ...);
static int initsock(void);
static void handleconn(int fd);

/* global variables */
static char *service = "http-alt";

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

int
initsock(void)
{
	struct addrinfo hints;
	struct addrinfo *res;
	int err, fd, one = 1;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_PASSIVE;
	if ((err = getaddrinfo(NULL, service, &hints, &res))) {
		fprintf(stderr, "getaddrinfo: %s\n", strerror(err));
		exit(EXIT_FAILURE);
	}

	fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	if (fd < 0)
		die("socket: %s", strerror(errno));

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)))
		die("setsockopt: %s", strerror(errno));

	if (bind(fd, res->ai_addr, res->ai_addrlen))
		die("bind: %s", strerror(errno));

	if (listen(fd, SOMAXCONN))
		die("listen: %s", strerror(errno));

	freeaddrinfo(res);
	return fd;
}

void
handleconn(int fd)
{
	char buf[BUFSIZ+1];

	bzero(buf, sizeof(buf));
	read(fd, buf, BUFSIZ);
	printf("received: %s", buf);
}

int
main(int argc, char *argv[1])
{
	int sockfd, tmpfd;
	pid_t pid;

	sockfd = initsock();

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
