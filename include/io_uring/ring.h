#ifndef RING_H_
    #define RING_H_

    #include <time.h>
    #include <linux/io_uring.h>
    #include <signal.h>

    #ifndef __cplusplus
    #include <stdatomic.h>
    #endif

    #define IS_SINGLE_MAP(p) (p.features & IORING_FEAT_SINGLE_MMAP)

typedef struct io_uring_params ring_params_t;
typedef struct app_io_ring_s {
    int ring_fd;

    unsigned *sq_head;
#ifdef __cplusplus
    unsigned volatile *sq_tail;
#else
    _Atomic unsigned volatile *sq_tail;
#endif
    unsigned *sq_mask;
    unsigned *sq_array;
    struct io_uring_sqe *sqes;

    unsigned *cq_head;
    unsigned *cq_tail;
    unsigned *cq_mask;
    struct io_uring_cqe *cqes;

    ring_params_t params;
} ring_t;

#ifdef __cplusplus
extern "C" {
#endif

ring_t *ring_init(unsigned depth, ring_params_t params);
void ring_destroy(ring_t *ring);
struct io_uring_sqe *ring_get_sqe(ring_t *ring);
void ring_register_sqe(ring_t *ring);
int ring_submit(ring_t *ring);
struct io_uring_cqe *ring_peek_cqe(ring_t *ring);
void ring_cqe_seen(ring_t *ring);
void ring_kernel_sqpoll_wakeup(ring_t *ring);

#ifdef __cplusplus
}
#endif

#endif /* RING_H_ */
