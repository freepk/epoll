#!/bin/bash
docker build -t centos-go .
docker run --cpus=4 --net host centos-go
