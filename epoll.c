#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_EVENTS 2048

char Http200[] = "HTTP/1.1 200 OK\r\n"
                 "Content-Length: 2\r\n"
                 "\r\n"
                 "{}";

void listenAndServe() {
	int srvSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (srvSock == -1) {
		return;
	}
	int one = 1;
	if (setsockopt(srvSock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) == -1) {
		return;
	}
	if (setsockopt(srvSock, SOL_TCP, TCP_NODELAY, &one, sizeof(one)) == -1) {
		return;
	}
	if (setsockopt(srvSock, SOL_TCP, TCP_DEFER_ACCEPT, &one, sizeof(one)) == -1) {
		return;
	}
	int queueLen = 2048;
	if (setsockopt(srvSock, SOL_TCP, TCP_FASTOPEN, &queueLen, sizeof(queueLen)) == -1) {
		return;
	}
	struct sockaddr_in srvAddr = { .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(80)};
	if (bind(srvSock, (struct sockaddr *) &srvAddr, sizeof(srvAddr)) == -1) {
		return;
	}
	if (listen(srvSock, SOMAXCONN) == -1) {
		return;
	}
	int epollFd = epoll_create1(0);
	if (epollFd == -1) {
		return;
	}
	struct epoll_event event = { .events = EPOLLIN, .data.fd = srvSock};
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, srvSock, &event) == -1) {
		return;
	}
	struct epoll_event events[MAX_EVENTS];
	char buf[16384];
	while(1) {
		int n = epoll_wait(epollFd, events, MAX_EVENTS, 0);
		if (n == -1) {
			return;
		}
		while(n > 0) {
			n--;
			int sock = events[n].data.fd;
			if (sock == srvSock) {
				sock = accept4(srvSock, NULL, NULL, SOCK_NONBLOCK);
				if (sock == -1) {
					return;
				}
				if (setsockopt(sock, SOL_TCP, TCP_NODELAY, &one, sizeof(one)) == -1) {
					return;
				}
				event.data.fd = sock;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, sock, &event) == -1) {
					return;
				}
			}
			int m = recv(sock, buf, sizeof(buf), 0);
			if (m == 0) {
				close(sock);
				continue;
			}
			if (m == -1) {
				close(sock);
				continue;
			}
			if (buf[0] == 'G' || buf[0] == 'P') {
				send(sock, Http200, sizeof(Http200) - 1, 0);
			}
		}
	}
}

void *serverWorker(void *args) {
        listenAndServe();
        return NULL;
}

int main() {
        pthread_t thr1, thr2, thr3;
        if (pthread_create(&thr1, NULL, serverWorker, NULL) != 0) {
                return -1;
        }
        if (pthread_create(&thr2, NULL, serverWorker, NULL) != 0) {
                return -1;
        }
        if (pthread_create(&thr3, NULL, serverWorker, NULL) != 0) {
                return -1;
        }
        pthread_join(thr1, NULL);
        pthread_join(thr2, NULL);
        pthread_join(thr3, NULL);
        return 0;
}

