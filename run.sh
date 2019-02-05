#!/bin/bash
docker build -t centos-go .
docker run -it -v $(pwd):/root/go/src/github.com/freepk/epoll centos-go
