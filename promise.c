#include "promise.h"

promise *promise_spawn(int fd, struct io_uring *uring, promise *caller, void *data, void (*func)(promise *p)) 
{
    promise *p = malloc(sizeof(*p));
    *p = (promise){
        .fd = fd,
        .uring = uring,
        .caller = caller,
        .state = 0,
        .ret = 0,
        .data = data,
        .func = func,
    };
    func(p);
    return p;
}

void promise_free(promise *p)
{
    free(p->data);
    free(p);
}