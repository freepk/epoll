FROM centos:7

WORKDIR /tmp

RUN yum install -y wget git vim telnet && \
	wget https://dl.google.com/go/go1.11.5.linux-amd64.tar.gz && \
	tar -C /usr/local -xzf go1.11.5.linux-amd64.tar.gz

WORKDIR /root

ENV PATH=${PATH}:/usr/local/go/bin GOROOT=/usr/local/go GOPATH=/root/go

