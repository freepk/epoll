#!/bin/bash
docker build -t gcc-epoll -f Dockerfile.gcc .
docker run -p 8888:8888 gcc-epoll
