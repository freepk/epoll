package main

import (
	"syscall"
)

var Http200 = []byte("HTTP/1.1 200 OK\r\n" +
	"Server: hlc\r\n" +
	"Content-Length: 2\r\n" +
	"\r\n" +
	"{}")

func listenAndServe() {
	srvSock, err := syscall.Socket(syscall.AF_INET, (syscall.SOCK_STREAM | syscall.SOCK_NONBLOCK), syscall.IPPROTO_TCP)
	if err != nil {
		panic(err)
	}
	if err = syscall.SetsockoptInt(srvSock, syscall.SOL_SOCKET, 0x0F /* SO_REUSEPORT */, 1); err != nil {
		panic(err)
	}
	if err = syscall.SetsockoptInt(srvSock, syscall.SOL_TCP, syscall.TCP_NODELAY, 1); err != nil {
		panic(err)
	}
	if err = syscall.SetsockoptInt(srvSock, syscall.SOL_TCP, syscall.TCP_DEFER_ACCEPT, 1); err != nil {
		panic(err)
	}
	if err = syscall.SetsockoptInt(srvSock, syscall.SOL_TCP, 0x17 /* TCP_FASTOPEN */, 2048 /* Queue length */); err != nil {
		panic(err)
	}
	if err = syscall.Bind(srvSock, &syscall.SockaddrInet4{Port: 80}); err != nil {
		panic(err)
	}
	if err = syscall.Listen(srvSock, syscall.SOMAXCONN); err != nil {
		panic(err)
	}
	epollFd, err := syscall.EpollCreate1(0)
	if err != nil {
		panic(err)
	}
	event := syscall.EpollEvent{Fd: int32(srvSock), Events: syscall.EPOLLIN}
	if err = syscall.EpollCtl(epollFd, syscall.EPOLL_CTL_ADD, srvSock, &event); err != nil {
		panic(err)
	}
	events := make([]syscall.EpollEvent, 2048)
	buf := make([]byte, 16384)
	for {
		n, err := syscall.EpollWait(epollFd, events, 0)
		if err != nil {
			panic(err)
		}
		for i := 0; i < n; i++ {
			sock := int(events[i].Fd)
			if sock == srvSock {
				if sock, _, err = syscall.Accept4(sock, syscall.SOCK_NONBLOCK); err != nil {
					panic(err)
				}
				if err = syscall.SetsockoptInt(sock, syscall.SOL_TCP, syscall.TCP_NODELAY, 1); err != nil {
					panic(err)
				}
				event = syscall.EpollEvent{Fd: int32(sock), Events: syscall.EPOLLIN}
				if err = syscall.EpollCtl(epollFd, syscall.EPOLL_CTL_ADD, sock, &event); err != nil {
					panic(err)
				}
			}
			m, err := syscall.Read(sock, buf)
			if m == 0 {
				syscall.Close(sock)
				continue
			}
			if err != nil {
				syscall.Close(sock)
				continue
			}
			if buf[0] == 'G' || buf[0] == 'P' {
				syscall.Write(sock, Http200)
			}
		}
	}
}

func main() {
	go listenAndServe()
	go listenAndServe()
	listenAndServe()
}
