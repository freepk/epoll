package main

import (
	"log"
	"syscall"
)

const SO_REUSEPORT = 0x0F

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
			case events[i].Events == syscall.EPOLLIN && events[i].Fd == int32(fd):
				//log.Println("Accept case")
				nfd, _, err := syscall.Accept4(fd, syscall.SOCK_NONBLOCK)
				if err != nil {
					log.Fatal("syscall.Accept4:", err)
				}
				event := &syscall.EpollEvent{Fd: int32(nfd), Events: syscall.EPOLLIN}
				if err = syscall.EpollCtl(efd, syscall.EPOLL_CTL_ADD, nfd, event); err != nil {
					log.Fatal("syscall.EpollCtl:", err)
				}
			case events[i].Events == syscall.EPOLLIN && events[i].Fd != int32(fd):
				//log.Println("Read case")
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
					//log.Println("syscall.Read:", string(buf[:n]))
				}
				syscall.Close(int(events[i].Fd))
			}
		}
	}
}

func main() {
	addr := &syscall.SockaddrInet4{Port: 8888}
	n := 4
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
