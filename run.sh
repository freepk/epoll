#!/bin/bash
docker build -t golang-epoll .
docker run --network="host" golang-epoll
