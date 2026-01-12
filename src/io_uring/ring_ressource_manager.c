#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <string.h>
#include "ring.h"

int io_uring_setup(unsigned entries, struct io_uring_params *p)
{
    return syscall(__NR_io_uring_setup, entries, p);
}

int io_uring_enter(int fd, unsigned to_submit, unsigned min_complete, unsigned flags, sigset_t *sig)
{
    return syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, sig, _NSIG / 8);
}

ring_t *ring_init(unsigned depth)
{
    if (depth == 0 || depth > 4096) {
        fprintf(stderr, "Invalid depth: %u\n", depth);
        return NULL;
    }

    ring_t *ring = malloc(sizeof(ring_t));
    ring_params_t params;
    void *sq_ptr, *cq_ptr;
    size_t sq_size, cq_size = 0;

    if (!ring) {
        perror("ring alloc");
        goto RING_ALLOC_ERROR;
    }

    memset(&params, 0, sizeof(params));

    ring->ring_fd = io_uring_setup(depth, &params);

    if (ring->ring_fd < 0) {
        perror("ring fd failure");
        goto RING_FD_FAIL;
    }

    if (params.sq_entries < depth) {
        fprintf(stderr, "Requested %u entries, kernel provided %u\n", depth, params.sq_entries);
    }

    ring->params = params;

    sq_size = params.sq_off.array + params.sq_entries * sizeof(unsigned);
    sq_ptr = mmap(
        NULL,
        sq_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_POPULATE,
        ring->ring_fd,
        IORING_OFF_SQ_RING
    );

    if (sq_ptr == MAP_FAILED) {
        perror("SQ map failure");
        goto SQ_MAPPING_FAIL;
    }

    if (IS_SINGLE_MAP(params))
        cq_ptr = sq_ptr;
    else {
        cq_size = params.cq_off.cqes + params.cq_entries * sizeof(struct io_uring_cqe);
        cq_ptr = mmap(
            NULL,
            cq_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE,
            ring->ring_fd,
            IORING_OFF_CQ_RING
        );

        if (cq_ptr == MAP_FAILED) {
            perror("CQ map failure");
            goto CQ_MAPPING_FAIL;
        }
    }

    ring->sqes = mmap(
        NULL,
        params.sq_entries * sizeof(struct io_uring_sqe),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_POPULATE,
        ring->ring_fd,
        IORING_OFF_SQES
    );

    if (ring->sqes == MAP_FAILED) {
        perror("SQEs map failure");
        goto SQES_MAPPING_FAIL;
    }

    ring->sq_head = sq_ptr + params.sq_off.head;
    ring->sq_tail = sq_ptr + params.sq_off.tail;
    ring->sq_mask = sq_ptr + params.sq_off.ring_mask;
    ring->sq_array = sq_ptr + params.sq_off.array;

    ring->cq_head = cq_ptr + params.cq_off.head;
    ring->cq_tail = cq_ptr + params.cq_off.tail;
    ring->cq_mask = cq_ptr + params.cq_off.ring_mask;
    ring->cqes = cq_ptr + params.cq_off.cqes;

    return ring;

// Error handling

SQES_MAPPING_FAIL:
    if (!(IS_SINGLE_MAP(params)))
        munmap(cq_ptr, cq_size);

CQ_MAPPING_FAIL:
    munmap(sq_ptr, sq_size);

SQ_MAPPING_FAIL:
    close(ring->ring_fd);

RING_FD_FAIL:
    free(ring);

RING_ALLOC_ERROR:
    return NULL;
}

void ring_destroy(ring_t *ring) {
    if (!ring) return;

    void *sq_ptr, *cq_ptr;
    unsigned sq_size, cq_size = 0;

    sq_ptr = (char *)ring->sq_mask - ring->params.sq_off.ring_mask;
    cq_ptr = (char *)ring->cq_mask - ring->params.cq_off.ring_mask;
    sq_size = ring->params.sq_off.array + ring->params.sq_entries * sizeof(unsigned);

    // unmap submission queue
    munmap(sq_ptr, sq_size);

    // checks for dual map
    if (!IS_SINGLE_MAP(ring->params)) {
        // unmap completion queue
        cq_size = ring->params.cq_off.cqes + ring->params.cq_entries * sizeof(struct io_uring_cqe);
        munmap(cq_ptr, cq_size);
    }

    munmap(ring->sqes, ring->params.sq_entries * sizeof(struct io_uring_sqe));
    close(ring->ring_fd);
    free(ring);
}
