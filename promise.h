#pragma once

#include <liburing.h>
#include <stdlib.h>

typedef struct promise promise;

struct promise
{
    int fd;
    struct io_uring *uring;
    promise *caller;

    int state;
    int ret;
    void *data;
    void (*func)(promise *p);
};

extern promise *promise_spawn(int fd, struct io_uring *uring, promise *caller, void *data, void (*func)(promise *p));

extern void promise_free(promise *g);
