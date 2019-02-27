#define _GNU_SOURCE
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_EVENTS 2048

int defaultPort = 8888;
char defaultResponse[90] = "HTTP/1.1 202 OK\r\n"
	"Server: Techlab\r\n"
	"Content-Type: application/json\r\n"
	"Content-Length: 2\r\n"
	"\r\n"
	"{}";
char getMethod[3] = "GET";
char postMethod[4] = "POST";

void handleInput(int epollfd, int fd) {
	int n;
	char buf[16384];

	n = read(fd, buf, sizeof(buf));
	if (n == 0) {
		epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		return;
	}
	if (n == -1) {
		epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		return;
	}
	if (strncmp(buf, getMethod, 3) != 0 && strncmp(buf, postMethod, 4) != 0) {
		return;
	}
	n = write(fd, defaultResponse, 90);
	if (n == -1) {
		perror("write failed");
		exit(EXIT_FAILURE);
	}
}

void *startLoop(void *dummy) {
	int listen_sock, client_sock;
	int one, n, i;
	int epollfd;
	struct sockaddr_in addr;
	struct epoll_event event, events[MAX_EVENTS];

	one = 1;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(defaultPort);

	listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listen_sock == -1) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) == -1) {
		perror("setsockopt SO_REUSEPORT failed");
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
		n = epoll_wait(epollfd, events, MAX_EVENTS, 0);
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
				if (setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) == -1) {
					perror("setsockopt TCP_NODELAY failed");
					exit(EXIT_FAILURE);
				}
				if (setsockopt(client_sock, IPPROTO_TCP, TCP_QUICKACK, &one, sizeof(one)) == -1) {
					perror("setsockopt TCP_QUICKACK failed");
					exit(EXIT_FAILURE);
				}
				event.events = EPOLLIN /*| EPOLLET*/;
				event.data.fd = client_sock;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock, &event) == -1) {
					perror("epoll_ctl failed");
					exit(EXIT_FAILURE);
				}
			} else {
				handleInput(epollfd, events[i].data.fd);
			}
		}
	}
	return NULL;
}

int main() {
	pthread_t thr1, thr2, thr3;
	if (pthread_create(&thr1, NULL, startLoop, NULL) != 0) {
		perror("pthread_create failed");
		exit(EXIT_FAILURE);
	}
	if (pthread_create(&thr2, NULL, startLoop, NULL) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
	}
	if (pthread_create(&thr3, NULL, startLoop, NULL) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
	}
	pthread_join(thr1, NULL);
	pthread_join(thr2, NULL);
	pthread_join(thr3, NULL);
	return 0;
}
