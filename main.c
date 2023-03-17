#include <arpa/inet.h>
#include <errno.h>
#include <liburing.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include "promise.h"
#include "async.h"

#define BACKLOG_LEN 1000
#define PORT 8080
#define URING_LEN 2048
#define MSG_LEN 200
#define NAME_LEN 100

typedef struct {
    struct sockaddr_in addr;
    char name[NAME_LEN];
    char msg[MSG_LEN];
} actual_echo_data;

void actual_echo_func(promise *p) 
{
    switch (p->state) {
    case 0: {
        async_recv(p, &((actual_echo_data *)p->data)->name, NAME_LEN);
        p->state = 1;
        return;
    }
    case 1: {
        actual_echo_data *data = (actual_echo_data *)p->data;
        sprintf(data->msg, "Hello %*s, your IP address is %s\n", p->ret, data->name, inet_ntoa(data->addr.sin_addr));
        async_send(p, data->msg, strlen(data->msg));
        p->state = 2;
        return;
    }
    }
    if (p->caller) {
        p->caller->func(p->caller);
    }
    promise_free(p);
}

typedef struct {
    struct sockaddr_in addr;
} echo_data;

void echo_func(promise *p) 
{
    while (true) {
        switch (p->state) {
        case 0: {
            actual_echo_data *data = malloc(sizeof(*data));
            *data = (actual_echo_data){
                .addr = ((echo_data *)p->data)->addr,
            };
            promise_spawn(p->fd, p->uring, p, data, actual_echo_func);
            p->state = 1;
            return;
        }
        case 1:
            printf("return from echo func\n");
            p->state = 0;
        }
    }
    promise_free(p);
}

typedef struct {
    struct sockaddr_in addr;
    socklen_t addr_len;
} accept_data;

void accept_func(promise *p) 
{
    switch (p->state) {
    case 0: {
        accept_data *data = (accept_data *)p->data;
        data->addr_len = sizeof(data->addr);
        async_accept(p, &data->addr, &data->addr_len);
        p->state = 1;
        return;
    }
    case 1: {
        int fd = p->ret;
        echo_data *data = malloc(sizeof(*data));
        *data = (echo_data){
            .addr = ((accept_data *)p->data)->addr,
        };
        promise_spawn(fd, p->uring, p, data, echo_func);
        p->state = 0;
        return;
    }
    }

    promise_free(p);
}

int main(int argc, char **argv)
{
    // create socket
    int listen_fd = listen_socket(8080, 1000);
    if (listen_fd < 0) {
        goto out_err;
    }

    // create uring
    struct io_uring ring;
    int ret = io_uring_queue_init(URING_LEN, &ring, 0);
    if (ret < 0)
    {
        perror("uring creation error");
        goto out_err;
    }

    // spawn accept promise
    accept_data *data = malloc(sizeof(*data));
    promise_spawn(listen_fd, &ring, NULL, data, accept_func);

    // event loop
    // wait for events and run the associated promises
    struct io_uring_cqe *cqe;
    while (true) {
        io_uring_submit(&ring);
        io_uring_wait_cqe(&ring, &cqe);
        if (cqe->res < 0) {
            printf("%d\n", cqe->res);
            perror("wait cqe error");
            goto out_err;
        } 
        promise *p = io_uring_cqe_get_data(cqe);
        p->ret = cqe->res;
        io_uring_cqe_seen(&ring, cqe);
        p->func(p);
    }

out_err:
    close(listen_fd);
    return 1;
}