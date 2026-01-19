#include <memory.h>
#include <sys/epoll.h>
#include "AsyncIORing.hpp"

AsyncIORing::AsyncIORing(submission_mode_t mode, size_t param)
{
    ring_params_t params;

    memset(&params, 0, sizeof(params));
    if (mode == AUTO_KERNEL_POLLING) {
        params.flags = IORING_SETUP_SQPOLL;
        params.sq_thread_idle = param;
    }
    if (mode == THRESHOLD) {
        this->submitThreshold_ = param;
    }
    ring_ = ring_init(4096, params);
    this->submissionMode_ = mode;
}

AsyncIORing::~AsyncIORing()
{
    if (this->queuedCount_ > 0)
        this->submit();
    if (running_)
        this->stopEventThread();
    ring_destroy(ring_);
    // do something else ?
}

size_t AsyncIORing::submit()
{
    size_t diff;
    ring_submit(this->ring_);
    diff = queuedCount_;
    queuedCount_ = 0;
    return diff;
}

void AsyncIORing::runEventThread()
{
    this->running_ = true;

    eventThread_ = std::thread([this]() {
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1)
            return;
        
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = this->ring_->ring_fd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->ring_->ring_fd, &ev) == -1) {
            ::close(epoll_fd);
            return;
        }

        struct epoll_event events[1];
        while (this->running_) {
            int nfds = epoll_wait(epoll_fd, events, 1, 100);
            if (nfds > 0) {
                std::lock_guard<std::mutex> lock(this->thread_mutex_);
                io_uring_cqe *cqe;
                while ((cqe = ring_peek_cqe(this->ring_))) {
                    size_t id = (size_t)cqe->user_data;
                    auto p = this->pending_.find(id);
                    if (p != this->pending_.end()) {
                        auto promise = p->second;
                        if (cqe->res >= 0)
                            promise->resolve({.result = cqe->res, .flags = cqe->flags});
                        else
                            promise->reject(cqe->res);
                        this->pending_.erase(p);
                    }
                    this->completedCount_++;
                    ring_cqe_seen(ring_);
                }
            }
        }
        ::close(epoll_fd);
    });
}

void AsyncIORing::stopEventThread()
{
    running_ = false;
    if (eventThread_.joinable())
        eventThread_.join();
}

IOPromisePtr AsyncIORing::queueOp(io_uring_sqe *sqe)
{
    size_t id = nextId_.fetch_add(1);
    IOPromisePtr promise = Promise<IOResult>::create();

    sqe->user_data = (unsigned long long)id;
    ring_register_sqe(this->ring_);
    this->pending_[id] = promise;
    this->queuedCount_++;
    if (this->submissionMode_ == THRESHOLD && this->queuedCount_ >= this->submitThreshold_)
        this->submit();
    if (this->submissionMode_ == AUTO_KERNEL_POLLING && this->ring_->params.sq_off.flags & IORING_SQ_NEED_WAKEUP)
        ring_kernel_sqpoll_wakeup(this->ring_);
    return promise;
}
