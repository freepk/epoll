FROM centos:7
WORKDIR /tmp
RUN yum install -y wget git && \
	wget https://dl.google.com/go/go1.11.5.linux-amd64.tar.gz && \
	tar -C /usr/local -xzf go1.11.5.linux-amd64.tar.gz
ENV PATH=${PATH}:/usr/local/go/bin GOROOT=/usr/local/go GOPATH=/root/go
RUN mkdir -p /root/go/src/github.com/freepk
WORKDIR /root/go/src/github.com/freepk
RUN git clone https://github.com/freepk/epoll.git && \
	cd epoll && \
	go clean && \
	go build
EXPOSE 8888
CMD ["/root/go/src/github.com/freepk/epoll/epoll"]
