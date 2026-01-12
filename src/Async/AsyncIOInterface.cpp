#include "AsyncIOInterface.hpp"
#include <iostream>
#include <cstring>

AsyncIOInterface::AsyncIOInterface(unsigned int depth, int auto_submit_threshold)
{
    this->ring = ring_init(depth);

    if (!this->ring)
        throw std::runtime_error("IO ring initialization failure");
    if (auto_submit_threshold == -1 || auto_submit_threshold >= depth)
        this->m_auto_submit = depth;
    else
        this->m_auto_submit = auto_submit_threshold;
}

AsyncIOInterface::~AsyncIOInterface()
{
    struct io_uring_cqe *cqe = NULL;

    while ((cqe = ring_peek_cqe(this->ring)) != NULL) {
        ring_cqe_seen(this->ring);
    }
    ring_destroy(this->ring);
    return;
}

void OPConstructor::register_op()
{
    m_interface.register_operation(m_sqe, m_handler_success, m_handler_failure);
}

std::size_t AsyncIOInterface::get_next_id()
{
    static std::size_t id = 0;
    return id++;
}

OPConstructor AsyncIOInterface::new_op()
{
    struct io_uring_sqe *sqe = ring_get_sqe(this->ring);

    if (!sqe)
        throw std::runtime_error("Submission queue is full");
    
    // Zero out the SQE
    memset(sqe, 0, sizeof(*sqe));
    
    return OPConstructor(*this, sqe);
}

void AsyncIOInterface::register_operation(io_uring_sqe *sqe, CQEHandler on_success, CQEHandler on_failure)
{
    // Note: We can't call ring_get_sqe again here because it zeros out the SQE!
    // The sqe pointer validity is ensured by the OPConstructor lifecycle
    std::size_t op_id = this->get_next_id();

    sqe->user_data = op_id;
    ring_register_sqe(this->ring);
    this->m_pending_sqes++;
    if (on_success || on_failure)
        this->m_ops.insert_or_assign(op_id, std::make_pair(on_success, on_failure));
    if (this->m_auto_submit != -1 && this->m_pending_sqes >= this->m_auto_submit)
        this->submit();
}

void AsyncIOInterface::process_completions(int nb)
{
    struct io_uring_cqe *cqe;
    while ((cqe = ring_peek_cqe(this->ring)) && (nb == -1 || nb > 0)) {
        std::size_t op_id = (std::size_t)cqe->user_data;
        auto op = this->m_ops.find(op_id);
        if (op != this->m_ops.end()) {
            if (cqe->res >= 0 && op->second.first) {
                op->second.first(cqe->res, cqe->flags);
            } else if (op->second.second) {
                op->second.second(cqe->res, cqe->flags);
            }
            this->m_ops.erase(op);
        }
        ring_cqe_seen(this->ring);
        this->m_seen_cqes++;
        nb = nb == -1 ? nb : nb--;
    }
}

std::size_t AsyncIOInterface::submit()
{
    std::size_t nb_submitions = m_pending_sqes;

    if (nb_submitions == 0)
        return 0;

    ring_submit(this->ring);
    m_submited_sqes += m_pending_sqes;
    m_pending_sqes = 0;
    return nb_submitions;
}

std::size_t AsyncIOInterface::submit_and_wait(std::size_t nb_wait)
{
    std::size_t nb_submitions = m_pending_sqes;

    if (nb_submitions == 0 || nb_wait > nb_submitions)
        return 0;

    io_uring_enter(this->ring->ring_fd, *this->ring->sq_tail - *this->ring->sq_head, nb_wait, 0, NULL);
    m_submited_sqes += m_pending_sqes;
    m_pending_sqes = 0;
    return nb_submitions;
}

void AsyncIOInterface::await_completions(int nb_wait)
{
    // Calculate how many completions are pending
    std::size_t pending_completions = m_submited_sqes - m_seen_cqes;
    
    // If nb_wait is -1, wait for all pending completions
    if (nb_wait == -1)
        nb_wait = pending_completions;
    
    if (nb_wait == 0 || pending_completions == 0)
        return;

    // Wait for completions in a loop until we've seen enough
    std::size_t initial_seen = m_seen_cqes;
    std::size_t target_seen = initial_seen + nb_wait;
    
    while (m_seen_cqes < target_seen) {
        // Wait for at least 1 completion
        io_uring_enter(this->ring->ring_fd, 0, 1, IORING_ENTER_GETEVENTS, NULL);
        
        // Process all available completions
        this->process_completions();
    }
}
