#ifndef ASYNC_IOINTERFACE_H
    #define ASYNC_OINTERFACE_H

#include <vector>
#include <thread>
#include <exception>
#include <stdexcept>
#include <functional>
#include <unordered_map>
#include "ring.h"

using CQEHandler = std::function<void (int res, int flags)>;

class AsyncIOInterface;

class OPConstructor {
    public:
        OPConstructor(AsyncIOInterface &interface, io_uring_sqe *sqe) : m_interface(interface), m_sqe(sqe) {}
        ~OPConstructor() = default;

        OPConstructor &op(io_uring_op op) { m_sqe->opcode = op; return *this; }
        OPConstructor &fd(int fd) { m_sqe->fd = fd; return *this; }
        OPConstructor &addr(void *addr) { m_sqe->addr = (unsigned long)addr; return *this; }
        OPConstructor &addr2(void *addr) { m_sqe->addr2 = (unsigned long)addr; return *this; }
        OPConstructor &len(unsigned int len) { m_sqe->len = len; return *this; }
        OPConstructor &offset(unsigned long long off) { m_sqe->off = off; return *this; }
        OPConstructor &flags(unsigned char flags) { m_sqe->flags = flags; return *this; }
        // Used to set any flags within the SQE since all OP flags are part of the same union
        OPConstructor &op_flags(unsigned int flags) { m_sqe->msg_flags = flags; return *this; }

        OPConstructor &OnSuccess(CQEHandler handler) { m_handler_success = handler; return *this; }
        OPConstructor &OnFailure(CQEHandler handler) { m_handler_failure = handler; return *this; }
        void register_op();
    private:
        CQEHandler m_handler_failure;
        CQEHandler m_handler_success;
        io_uring_sqe *m_sqe;
        AsyncIOInterface &m_interface;
};

class AsyncIOInterface {
    public:
        AsyncIOInterface(unsigned int depth = 4096, int auto_submit_threshold = -1);
        ~AsyncIOInterface();

        OPConstructor new_op();

        void register_operation(io_uring_sqe *sqe, CQEHandler on_success = nullptr, CQEHandler on_failure = nullptr);
        void process_completions(int nb = -1);
        std::size_t submit();
        std::size_t submit_and_wait(std::size_t nb_wait);
        void await_completions(int nb_wait = -1);
    protected:
    private:
        std::size_t get_next_id();
        std::size_t m_submited_sqes = 0;
        std::size_t m_pending_sqes = 0;
        std::size_t m_seen_cqes = 0;
        std::unordered_map<std::size_t, std::pair<CQEHandler, CQEHandler>> m_ops;
        ring_t *ring;
        int m_auto_submit = -1;
};

#endif // AsyncIOINTERFACE_H
