#define _GNU_SOURCE
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 128

void handle_input(int fd) {
	int n;
	char buf[16384];

	n = read(fd, buf, sizeof(buf));
	if (n == 0) {
		close(fd);
		return;
	}
	write(fd, "HTTP/1.1 200 OK\r\n\r\nHello World", 30);
}

void start_loop(void) {
	int listen_sock, client_sock;
	int one, n, i;
	int epollfd;
	struct sockaddr_in addr;
	struct epoll_event event, events[MAX_EVENTS];

	listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listen_sock == -1) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	one = 1;
	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) == -1) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(8888);
	if (bind(listen_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(listen_sock, SOMAXCONN) == -1) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}
	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1 failed");
		exit(EXIT_FAILURE);
	}
	event.events = EPOLLIN
	event.data.fd = listen_sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
		perror("epoll_ctl failed");
		exit(EXIT_FAILURE);
	}
	for (;;) {
		int n = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (n == -1) {
			perror("epoll_wait failed");
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < n; i++) {
			if (event[i].data.fd == listen_sock) {
				client_sock = accept4(listen_sock, NULL, NULL, SOCK_NONBLOCK);
				if (client_sock == -1) {
					perror("accept failed");
					exit(EXIT_FAILURE);
				}
				event.events = EPOLLIN | EPOLLET;
				event.data.fd = client_sock;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock, &event) == -1) {
					perror("epoll_ctl failed");
					exit(EXIT_FAILURE);
				}
			} else {
				handle_input(event[i].data.fd);
			}
		}
	}
}

int main(void) {
	start_loop();
	return 0;
}
