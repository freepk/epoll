package main

import (
	"log"
	"syscall"
)

const SO_REUSEPORT = 0x0F

var DefaultResponse = []byte("HTTP/1.0 200 OK\r\n" +
	"Connection: Keep-Alive\r\n" +
	"Content-type: text/html\r\n" +
	"Connection: close\r\n" +
	"Content-Length: 2\r\n" +
	"\r\n" +
	"\r\n")

func wait(fd, efd int) {
	log.Println("wait", fd, efd)
	events := make([]syscall.EpollEvent, 128)
	buf := make([]byte, 8192)
	for {
		n, err := syscall.EpollWait(efd, events, -1)
		if err != nil {
			log.Fatal("syscall.EpollWait:", err)
		}
		for i := 0; i < n; i++ {
			switch {
			case events[i].Fd == int32(fd):
				//log.Println("Listener case")
				nfd, _, err := syscall.Accept4(fd, syscall.SOCK_NONBLOCK)
				if err != nil {
					log.Fatal("syscall.Accept4:", err)
				}
				event := &syscall.EpollEvent{Fd: int32(nfd), Events: syscall.EPOLLIN}
				if err = syscall.EpollCtl(efd, syscall.EPOLL_CTL_ADD, nfd, event); err != nil {
					log.Fatal("syscall.EpollCtl:", err)
				}
			case events[i].Fd != int32(fd):
				//log.Println("Client case")
				total := 0
				for {
					n, err := syscall.Read(int(events[i].Fd), buf)
					if err == syscall.EAGAIN {
						//log.Println("syscall.Read:EAGAIN", n)
						break
					}
					if err != nil {
						log.Fatal("syscall.Read:", err)
					}
					if n == 0 {
						break
					}
					total += n
					//log.Println("syscall.Read:", string(buf[:n]))
				}
				if total == 0 {
					syscall.Close(int(events[i].Fd))
					continue
				}
				n, err := syscall.Write(int(events[i].Fd), DefaultResponse)
				if n != len(DefaultResponse) {
					log.Fatal("syscall.Write: data corruption")
				}
				if err != nil {
					log.Fatal("syscall.Write:", err)
				}
			}
		}
	}
}

func main() {
	addr := &syscall.SockaddrInet4{Port: 8888}
	n := 1
	for i := 0; i < n; i++ {
		fd, err := syscall.Socket(syscall.AF_INET, syscall.SOCK_STREAM, syscall.IPPROTO_TCP)
		if err != nil {
			log.Fatal("syscall.Socket:", err)
		}
		if err = syscall.SetNonblock(fd, true); err != nil {
			log.Fatal("syscall.SetNonblock:", err)
		}
		if err = syscall.SetsockoptInt(fd, syscall.SOL_SOCKET, SO_REUSEPORT, 1); err != nil {
			log.Fatal("syscall.SetsockoptInt:", err)
		}
		if err = syscall.Bind(fd, addr); err != nil {
			log.Fatal("syscall.Bind:", err)
		}
		if err = syscall.Listen(fd, syscall.SOMAXCONN); err != nil {
			log.Fatal("syscall.Listen:", err)
		}
		efd, err := syscall.EpollCreate1(0)
		if err != nil {
			log.Fatal("syscall.EpollCreate1:", err)
		}
		event := &syscall.EpollEvent{Fd: int32(fd), Events: syscall.EPOLLIN}
		if err = syscall.EpollCtl(efd, syscall.EPOLL_CTL_ADD, fd, event); err != nil {
			log.Fatal("syscall.EpollCtl:", err)
		}
		go wait(fd, efd)
	}
	var ch chan struct{}
	<-ch
}
