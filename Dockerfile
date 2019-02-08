FROM golang:alpine as builder
RUN apk add unzip curl
RUN mkdir -p /root/go/src/github.com/freepk
WORKDIR /root/go/src/github.com/freepk
RUN curl -O https://codeload.github.com/freepk/epoll/zip/master && \
	unzip master && \
	cd epoll-master && \
	go clean && \
	CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -ldflags '-extldflags "-static"' -o epoll epoll.go

FROM scratch
COPY --from=builder /root/go/src/github.com/freepk/epoll-master/epoll /epoll
ENV GOGC=off
CMD ["/epoll"]
