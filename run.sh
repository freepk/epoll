#!/bin/bash
docker build -t centos-go .
docker run --net host -p 8888:8888 centos-go
