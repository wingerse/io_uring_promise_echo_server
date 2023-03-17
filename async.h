#include "promise.h"
#include <arpa/inet.h>
#include <stdint.h>

int listen_socket(uint16_t port, int backlog_size);
void async_accept(promise *p, struct sockaddr_in *addr, socklen_t *addr_len);
void async_send(promise *p, void *buf, size_t len);
void async_recv(promise *p, void *buf, size_t len);