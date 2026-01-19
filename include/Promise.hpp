#ifndef PROMISE_HPP_
#define PROMISE_HPP_

#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <string>

template<typename T>
class Promise {
    public:
        enum class State { Pending, Resolved, Rejected };

        static std::shared_ptr<Promise<T>> create()
        {
            return std::make_shared<Promise<T>>();
        }

        template<typename Executor>
        static std::shared_ptr<Promise<T>> create(Executor&& executor)
        {
            auto promise = std::make_shared<Promise<T>>();
            executor(
                [promise](T val) { promise->resolve(std::move(val)); },
                [promise](int err) { promise->reject(err); }
            );
            return promise;
        }

        void resolve(T&& val)
        {
            std::vector<std::function<void(T&)>> thenCbs;
            std::vector<std::function<void(T&, int)>> finallyCbs;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (state != State::Pending)
                    return;
                state = State::Resolved;
                value = std::move(val);
                thenCbs = std::move(thenCallbacks);
                finallyCbs = std::move(finallyCallbacks);
            }
            cv_.notify_all();
            for (auto &cb : thenCbs)
                cb(value);
            for (auto &cb : finallyCbs)
                cb(value, error);
        }

        void reject(int err)
        {
            std::vector<std::function<void(int)>> catchCbs;
            std::vector<std::function<void(T&, int)>> finallyCbs;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (state != State::Pending)
                    return;
                state = State::Rejected;
                error = err;
                catchCbs = std::move(catchCallbacks);
                finallyCbs = std::move(finallyCallbacks);
            }
            cv_.notify_all();
            for (auto &cb : catchCbs)
                cb(error);
            for (auto &cb : finallyCbs)
                cb(value, error);
        }

        Promise& then(std::function<void(T&)> cb)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (state == State::Resolved) {
                lock.unlock();
                cb(value);
            } else if (state == State::Pending) {
                thenCallbacks.push_back(std::move(cb));
            }
            return *this;
        }

        Promise& catch_(std::function<void(int)> cb)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (state == State::Rejected) {
                int err = error;
                lock.unlock();
                cb(err);
            } else if (state == State::Pending) {
                catchCallbacks.push_back(std::move(cb));
            }
            return *this;
        }

        Promise& finally(std::function<void(T&, int)> cb)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (state != State::Pending) {
                lock.unlock();
                cb(value, error);
            } else {
                finallyCallbacks.push_back(std::move(cb));
            }
            return *this;
        }

        T& await()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return state != State::Pending; });
            if (state == State::Rejected)
                throw std::runtime_error("Promise rejected with error: " + std::to_string(error));
            return value;
        }

        bool isPending() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return state == State::Pending;
        }

        bool isResolved() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return state == State::Resolved;
        }

        bool isRejected() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return state == State::Rejected;
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        State state = State::Pending;
        T value{};
        int error = 0;
        std::vector<std::function<void(T&)>> thenCallbacks;
        std::vector<std::function<void(int)>> catchCallbacks;
        std::vector<std::function<void(T&, int)>> finallyCallbacks;
};

#endif /* PROMISE_HPP_ */
