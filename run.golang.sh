#!/bin/bash
docker build -t golang-epoll -f Dockerfile.golang .
docker run -p 8888:8888 golang-epoll
