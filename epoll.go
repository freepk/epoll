package main

import (
	"log"
	"syscall"
)

const SO_REUSEPORT = 0x0F
const EPOLLET = 0x80000000

var DefaultResponse = []byte("HTTP/1.1 200 OK\r\n" +
	"Content-Length: 11\r\n" +
	"\r\n" +
	"Hello World")

func startLoop() {
	addr := &syscall.SockaddrInet4{Port: 8888}
	listenSock, err := syscall.Socket(syscall.AF_INET, syscall.SOCK_STREAM|syscall.SOCK_NONBLOCK, syscall.IPPROTO_TCP)
	if err != nil {
		log.Fatal("syscall.Socket:", err)
	}
	if err = syscall.SetsockoptInt(listenSock, syscall.SOL_SOCKET, SO_REUSEPORT, 1); err != nil {
		log.Fatal("syscall.SetsockoptInt:", err)
	}
	if err = syscall.Bind(listenSock, addr); err != nil {
		log.Fatal("syscall.Bind:", err)
	}
	if err = syscall.Listen(listenSock, syscall.SOMAXCONN); err != nil {
		log.Fatal("syscall.Listen:", err)
	}
	epollFd, err := syscall.EpollCreate1(0)
	if err != nil {
		log.Fatal("syscall.EpollCreate1:", err)
	}
	event := syscall.EpollEvent{Fd: int32(listenSock), Events: syscall.EPOLLIN}
	if err = syscall.EpollCtl(epollFd, syscall.EPOLL_CTL_ADD, listenSock, &event); err != nil {
		log.Fatal("syscall.EpollCtl:", err)
	}
	events := make([]syscall.EpollEvent, 16384)
	buf := make([]byte, 16384)
	for {
		n, err := syscall.EpollWait(epollFd, events, -1)
		if err != nil {
			log.Fatal("syscall.EpollWait:", err)
		}
		for i := 0; i < n; i++ {
			switch {
			case events[i].Fd == int32(listenSock):
				clientSock, _, err := syscall.Accept4(listenSock, syscall.SOCK_NONBLOCK)
				if err != nil {
					log.Fatal("syscall.Accept4:", err)
				}
				event = syscall.EpollEvent{Fd: int32(clientSock), Events: syscall.EPOLLIN}
				if err = syscall.EpollCtl(epollFd, syscall.EPOLL_CTL_ADD, clientSock, &event); err != nil {
					log.Fatal("syscall.EpollCtl:", err)
				}
			case events[i].Fd != int32(listenSock):
				fd := int(events[i].Fd)
				m, err := syscall.Read(fd, buf)
				if m == 0 {
					syscall.EpollCtl(epollFd, syscall.EPOLL_CTL_DEL, fd, nil)
					syscall.Close(fd)
					continue
				}
				if err != nil {
					syscall.EpollCtl(epollFd, syscall.EPOLL_CTL_DEL, fd, nil)
					syscall.Close(fd)
					continue
				}
				m, err = syscall.Write(fd, DefaultResponse)
				if err != nil {
					log.Fatal("syscall.Write:", err)
				}
			}
		}
	}
}

func main() {
	go startLoop()
	go startLoop()
	startLoop()
}
