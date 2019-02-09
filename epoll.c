#define _GNU_SOURCE
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_EVENTS 128

static char reply[50] = "HTTP/1.1 200 OK\r\n"
	"Content-Length: 11\r\n"
	"\r\n"
	"Hello World";

void handle_input(int fd) {
	int n;
	char buf[16384];

	n = read(fd, buf, sizeof(buf));
	if (n == 0) {
		close(fd);
		return;
	}
	n = write(fd, reply, 50);
	if (n == -1) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
}

void *start_loop(void *dummy) {
	int listen_sock, client_sock;
	int one, n, i;
	int epollfd;
	struct sockaddr_in addr;
	struct epoll_event event, events[MAX_EVENTS];

	one = 1;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(8888);

	listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listen_sock == -1) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) == -1) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}
	if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
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
	event.events = EPOLLIN;
	event.data.fd = listen_sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
		perror("epoll_ctl failed");
		exit(EXIT_FAILURE);
	}
	for (;;) {
		n = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (n == -1) {
			perror("epoll_wait failed");
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < n; i++) {
			if (events[i].data.fd == listen_sock) {
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
				handle_input(events[i].data.fd);
			}
		}
	}
	return NULL;
}

int main() {
	pthread_t thr1, thr2;
	if (pthread_create(&thr1, NULL, start_loop, NULL) != 0) {
		perror("pthread_create failed");
		exit(EXIT_FAILURE);
	}
	if (pthread_create(&thr2, NULL, start_loop, NULL) != 0) {
                perror("pthread_create failed");
                exit(EXIT_FAILURE);
	}
	pthread_join(thr1, NULL);
	pthread_join(thr2, NULL);
	return 0;
}
