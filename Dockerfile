FROM golang:alpine as builder
RUN mkdir -p /root/go/src/github.com/freepk/epoll
WORKDIR /root/go/src/github.com/freepk/epoll
ADD ./epoll.go ./
RUN go clean && CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -ldflags '-extldflags "-static"' -o epoll epoll.go

FROM scratch
COPY --from=builder /root/go/src/github.com/freepk/epoll/epoll /epoll
ENV GOGC=off
CMD ["/epoll"]
