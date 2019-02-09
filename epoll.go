package main

import (
	"log"
	"runtime"
	"syscall"
)

const SO_REUSEPORT = 0x0F
const EPOLLET = 0x80000000

var DefaultResponse = []byte("HTTP/1.1 200 OK\r\n" +
	"Content-Length: 11\r\n" +
	"\r\n" +
	"Hello World")

func start_loop() {
	addr := &syscall.SockaddrInet4{Port: 8888}
	listen_sock, err := syscall.Socket(syscall.AF_INET, syscall.SOCK_STREAM|syscall.SOCK_NONBLOCK, syscall.IPPROTO_TCP)
	if err != nil {
		log.Fatal("syscall.Socket:", err)
	}
	if err = syscall.SetsockoptInt(listen_sock, syscall.SOL_SOCKET, SO_REUSEPORT, 1); err != nil {
		log.Fatal("syscall.SetsockoptInt:", err)
	}
	if err = syscall.Bind(listen_sock, addr); err != nil {
		log.Fatal("syscall.Bind:", err)
	}
	if err = syscall.Listen(listen_sock, syscall.SOMAXCONN); err != nil {
		log.Fatal("syscall.Listen:", err)
	}
	epollfd, err := syscall.EpollCreate1(0)
	if err != nil {
		log.Fatal("syscall.EpollCreate1:", err)
	}
	event := syscall.EpollEvent{Fd: int32(listen_sock), Events: syscall.EPOLLIN}
	if err = syscall.EpollCtl(epollfd, syscall.EPOLL_CTL_ADD, listen_sock, &event); err != nil {
		log.Fatal("syscall.EpollCtl:", err)
	}
	events := make([]syscall.EpollEvent, 128)
	buf := make([]byte, 16384)
	for {
		n, err := syscall.EpollWait(epollfd, events, -1)
		if err != nil {
			log.Fatal("syscall.EpollWait:", err)
		}
		for i := 0; i < n; i++ {
			switch {
			case events[i].Fd == int32(listen_sock):
				client_sock, _, err := syscall.Accept4(listen_sock, syscall.SOCK_NONBLOCK)
				if err != nil {
					log.Fatal("syscall.Accept4:", err)
				}
				event = syscall.EpollEvent{Fd: int32(client_sock), Events: syscall.EPOLLIN | EPOLLET}
				if err = syscall.EpollCtl(epollfd, syscall.EPOLL_CTL_ADD, client_sock, &event); err != nil {
					log.Fatal("syscall.EpollCtl:", err)
				}
			case events[i].Fd != int32(listen_sock):
				m, err := syscall.Read(int(events[i].Fd), buf)
				if m == 0 {
					syscall.Close(int(events[i].Fd))
					continue
				}
				if err != nil {
					syscall.Close(int(events[i].Fd))
					continue
				}
				m, err = syscall.Write(int(events[i].Fd), DefaultResponse)
				if err != nil {
					log.Fatal("syscall.Write:", err)
				}
			}
		}
	}
}

func main() {
	n := runtime.NumCPU()
	for i := 0; i < n; i++ {
		go start_loop()
	}
	var ch chan struct{}
	<-ch
}
