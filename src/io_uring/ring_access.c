#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ring.h"

struct io_uring_sqe *ring_get_sqe(ring_t *ring)
{
    unsigned tail = *ring->sq_tail;
    unsigned head = *ring->sq_head;
    unsigned index;
    struct io_uring_sqe *sqe = NULL;

    if (tail - head >= ring->params.sq_entries)
        return NULL;
    index = tail & *ring->sq_mask;
    sqe = &ring->sqes[index];
    memset(sqe, 0, sizeof(*sqe));
    return sqe;
}

void ring_register_sqe(ring_t *ring)
{
    unsigned tail = *ring->sq_tail;
    unsigned index = tail & *ring->sq_mask;

    ring->sq_array[index] = index;
    __sync_synchronize();
    *ring->sq_tail = tail + 1;
}

int ring_submit(ring_t *ring)
{
    unsigned to_submit = *ring->sq_tail - *ring->sq_head;
    int ret;

    if (to_submit == 0)
        return 0;
    ret = io_uring_enter(ring->ring_fd, to_submit, 0, 0, NULL);
    if (ret < 0) {
        perror("io_uring_enter");
        return -1;
    }
    return ret;
}

struct io_uring_cqe *ring_peek_cqe(ring_t *ring)
{
    unsigned head = *ring->cq_head;
    unsigned tail = *ring->cq_tail;
    unsigned index;
    struct io_uring_cqe *cqe;

    if (head == tail)
        return NULL;
    __sync_synchronize();
    index = head & *ring->cq_mask;
    cqe = &ring->cqes[index];
    return cqe;
}

void ring_cqe_seen(ring_t *ring)
{
    __sync_synchronize();
    *ring->cq_head = *ring->cq_head + 1;
}
