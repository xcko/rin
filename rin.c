/*
 * Rin create's a simple network connection to a specified host
 * and then creates fifo in file and out file for input and output,
 * respectively. The goal is to allow simple debugging like netcat,
 * but not be limited by unidirectional pipes.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char	*init_dir(char *, char *);
static int	poll_handler(struct pollfd *, int, int, int);
static int	tcpopen(char *, char *);
static void	loop_conn(char *, int);
static void	rinrun(char *, char *, char *);
static void	usage(void);

static int
poll_handler(struct pollfd *pfd, int srv, int in_fd, int out_fd)
{
	int i, nfds = 2;

	for (i = 0; i < nfds; i++) {
		char buf[256];
		ssize_t bufl;

		if (pfd[i].revents & (POLLERR|POLLNVAL))
			errx(1, "bad fd %d", pfd[i].fd);
		if (pfd[i].revents & POLLIN) {
			bufl = read(pfd[i].fd, buf, sizeof buf);
			if (bufl == -1)
				err(1, "read");
			if (bufl == 0) {
				if (close(in_fd) == -1)
					err(1, "close");
				in_fd = -1;
			}
			if (pfd[i].fd == in_fd) {
				if (write(srv, buf, bufl) == -1)
					err(1, "write");
			} else if (pfd[i].fd == srv) {
				if (write(out_fd, buf, bufl) == -1)
					err(1, "write");
			}
		}
	}
	return in_fd;
}

static void
loop_conn(char *path, int srv)
{
	struct pollfd pfd[2];
	char *fifo, *file;
	int in_fd, out_fd;

	if (asprintf(&fifo, "%s/in", path) == -1)
		err(1, "asprintf");
	in_fd = open(fifo, O_RDONLY|O_NONBLOCK);
	if (in_fd == -1)
		err(1, "open");

	pfd[0].fd = in_fd;
	pfd[0].events = POLLIN;
	pfd[1].fd = srv;
	pfd[1].events = POLLIN;

	if (asprintf(&file, "%s/out", path) == -1)
		err(1, "asprintf");
	out_fd = open(file, O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
	if (out_fd == -1)
		err(1, "open");

	for (;;) {
		if (poll(pfd, 2, -1) == -1)	/* block */
			err(1, "poll");

		/* the fifo must be reopened after reading */
		in_fd = poll_handler(pfd, srv, in_fd, out_fd);
		if (in_fd == -1) {
			in_fd = open(fifo, O_RDONLY|O_NONBLOCK);
			if (in_fd == -1)
				err(1, "open");
			pfd[0].fd = in_fd;
		}
	}
	close(srv), close(in_fd), close(out_fd);
}

static int
tcpopen(char *host, char *port)
{
	struct addrinfo hints, *res, *res0;
	int error, save_errno;
	int s;
	const char *cause = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(host, port, &hints, &res0);
	if (error)
		errx(1, "%s", gai_strerror(error));
	s = -1;
	for (res = res0; res; res = res->ai_next) {
		s = socket(res->ai_family, res->ai_socktype,
			res->ai_protocol);
		if (s == -1) {
			cause = "socket";
			continue;
		}

		if (connect(s, res->ai_addr, res->ai_addrlen) == -1) {
			cause = "connect";
			save_errno = errno;
			close(s);
			errno = save_errno;
			s = -1;
			continue;
		}

		break;	/* okay we got one */
	}
	if (s == -1)
		err(1, "%s", cause);
	freeaddrinfo(res0);
	return s;
}

static char *
init_dir(char *dir, char *host)
{
	char *path, *file;
	int res;

	res = mkdir(dir, S_IRWXU);
	if (res == -1 && errno != EEXIST)
		err(1, "mkdir");
	else
		errno = 0;	/* reset errno on sucess */

	if (asprintf(&path, "%s/%s", dir, host) == -1)
		err(1, "asprintf");
	res = mkdir(path, S_IRWXU);
	if (res == -1 && errno != EEXIST)
		err(1, "mkdir");
	else
		errno = 0;

	if (asprintf(&file, "%s/in", path) == -1)
		err(1, "asprintf");
	res = mkfifo(file, S_IRUSR|S_IWUSR);
	if (res == -1 && errno != EEXIST)
		err(1, "mkfifo");
	else
		errno = 0;

	free(file);
	return path;
}

static void
rinrun(char *dir, char *host, char *port)
{
	char *path;
	int srv;

	path = init_dir(dir, host);
	srv = tcpopen(host, port);

	loop_conn(path, srv);
}

static void
usage(void)
{
	fprintf(stderr, "rin [-d <dir>] <host> <port>\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	char *dir, *host, *port;
	int ch;

	if (asprintf(&dir, "%s/rin", getenv("HOME")) == -1)
		err(1, "asprintf");

	while ((ch = getopt(argc, argv, "d:h")) != -1) {
		switch (ch) {
		case 'd':
			dir = strdup(optarg);
			if (dir == NULL)
				err(1, "strdup");
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;
	if (argc < 2)
		usage();

	host = strdup(argv[0]);
	if (host == NULL)
		err(1, "strdup");

	port = strdup(argv[1]);
	if (port == NULL)
		err(1, "strdup");

	rinrun(dir, host, port);

	return EXIT_SUCCESS;
}

