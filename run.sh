#!/bin/bash
docker build -t centos-go .
docker run -p 8888:8888 centos-go
