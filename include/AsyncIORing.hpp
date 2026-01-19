#ifndef ASYNCIORING_HPP_
#define ASYNCIORING_HPP_

#include <thread>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "io_uring/ring.h"
#include "Promise.hpp"
#include <sys/uio.h>
#include <sys/socket.h>

struct IOResult {
    int result;
    unsigned int flags;

    bool ok() const { return result >= 0; }
    int bytes() const { return result >= 0 ? result : 0; }
    int error() const { return result < 0 ? -result : 0; }
};

enum submission_mode_t {
    MANUAL,
    THRESHOLD,
    AUTO_KERNEL_POLLING
};

using IOPromisePtr = std::shared_ptr<Promise<IOResult>>;

class AsyncIORing {
    public:
        explicit AsyncIORing(submission_mode_t mode = MANUAL, size_t param = 0);
        ~AsyncIORing();

        AsyncIORing(const AsyncIORing&) = delete;
        AsyncIORing& operator=(const AsyncIORing&) = delete;

        IOPromisePtr read(int fd, void *buf, size_t len, off_t offset = 0);
        IOPromisePtr write(int fd, const void *buf, size_t len, off_t offset = 0);
        IOPromisePtr readv(int fd, const iovec *iov, int count, off_t offset = 0);
        IOPromisePtr writev(int fd, const iovec *iov, int count, off_t offset = 0);
        IOPromisePtr fsync(int fd);
        IOPromisePtr fdatasync(int fd);
        IOPromisePtr accept(int sockfd, sockaddr *addr, socklen_t *addrlen);
        IOPromisePtr connect(int sockfd, const sockaddr *addr, socklen_t addrlen);
        IOPromisePtr send(int sockfd, const void *buf, size_t len, int flags = 0);
        IOPromisePtr recv(int sockfd, void *buf, size_t len, int flags = 0);
        IOPromisePtr close(int fd);
        IOPromisePtr nop();

        size_t submit();

        size_t poll();

        size_t wait(size_t count = 1);
        size_t waitAll();

        void runEventThread();
        void stopEventThread();

        size_t pending() const { return this->queuedCount_; }
        size_t inflight() const { return this->submittedCount_ - this->completedCount_; }
        int fd() const { return this->ring_->ring_fd; }
        int eventFd() const;
    private:
        IOPromisePtr queueOp(io_uring_sqe *sqe);
        void processCompletion(io_uring_cqe *cqe);

        ring_t *ring_;
        int eventFd_ = -1;

        std::atomic<size_t> nextId_{0};
        std::unordered_map<size_t, IOPromisePtr> pending_;

        size_t queuedCount_ = 0;
        size_t submittedCount_ = 0;
        size_t completedCount_ = 0;

        std::atomic<bool> running_{false};
        std::mutex thread_mutex_;
        std::thread eventThread_;

        size_t submitThreshold_ = 0;
        submission_mode_t submissionMode_;
};

#endif /* ASYNCIORING_HPP_ */
