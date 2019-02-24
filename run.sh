#!/bin/bash
docker build -t golang-epoll .
docker run -p 8888:8888 golang-epoll
