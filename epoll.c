#define _GNU_SOURCE
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

char reply[] = "HTTP/1.1 200 OK\r\n"
	"Content-Type: text/plain\r\n"
	"Content-Length: 12\r\n"
	"\r\n"
	"Hello World!";

void start_loop()
{
	int sfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sfd == -1) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	int one = 1;
	int ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
	if (ret == -1) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}
	struct sockaddr_in addr = { .sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(8888) };
	ret = bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	ret = listen(sfd, SOMAXCONN);
	if (ret == -1) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}
	int efd = epoll_create1(0);
	if (efd == -1) {
		perror("epoll_create1 failed");
		exit(EXIT_FAILURE);
	}
	struct epoll_event event = { .events = EPOLLIN, .data.fd = sfd };
	ret = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
	if (ret == -1) {
		perror("epoll_ctl failed");
		exit(EXIT_FAILURE);
	}
	struct epoll_event events[SOMAXCONN];
	char request[32768];
	while (1) {
		int n = epoll_wait(efd, events, SOMAXCONN, 0);
		if (n == -1) {
			perror("epoll_wait failed");
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < n; i++) {
			int fd = events[i].data.fd;
			if (fd == sfd) {
				int nfd = accept4(sfd, NULL, NULL, SOCK_NONBLOCK);
				if (nfd == -1) {
					perror("accept failed");
					exit(EXIT_FAILURE);
				}
				event.data.fd = nfd;
				ret = epoll_ctl(efd, EPOLL_CTL_ADD, nfd, &event);
				if (ret == -1) {
					perror("epoll_ctl failed");
					exit(EXIT_FAILURE);
				}
			} else {
				int m = read(fd, request, sizeof(request));
				if (m == 0) {
					close(fd);
					continue;
				}
				m = write(fd, reply, sizeof(reply) - 1);
				if (m == -1) {
					perror("write failed");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
}

int main(void)
{
	start_loop();
	return 0;
}
