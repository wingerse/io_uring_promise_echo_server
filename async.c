#include "async.h"
#include <sys/socket.h>
#include <liburing.h>
#include <stdio.h>

int listen_socket(uint16_t port, int backlog_size) 
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket create error");
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr = {
            .s_addr = INADDR_ANY
        },
        .sin_port = htons(port)
    };


    int res = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (res < 0) {
        perror("bind error");
        return -1;
    }

    res = listen(fd, backlog_size);
    if (res < 0) {
        perror("listen error");
        return -1;
    }

    return fd;
}

void async_accept(promise *p, struct sockaddr_in *addr, socklen_t *addr_len) 
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(p->uring);
    io_uring_sqe_set_data(sqe, p);
    io_uring_prep_accept(sqe, p->fd, (struct sockaddr *)addr, addr_len, 0);
}

void async_send(promise *p, void *buf, size_t len) 
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(p->uring);
    io_uring_sqe_set_data(sqe, p);
    io_uring_prep_send(sqe, p->fd, buf, len, 0);
}

void async_recv(promise *p, void *buf, size_t len) 
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(p->uring);
    io_uring_sqe_set_data(sqe, p);
    io_uring_prep_recv(sqe, p->fd, buf, len, 0);
}