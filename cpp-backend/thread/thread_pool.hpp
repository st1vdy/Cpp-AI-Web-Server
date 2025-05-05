//
// Created by dongye on 25-5-3.
//

#pragma once
#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count = 4) : stop_(false) {
        for (size_t i = 0; i < thread_count; i++) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->mutex_);
                        this->cond_.wait(lock, [this] { return this->stop_ || !this->tasks_.empty(); });
                        if (this->stop_ && this->tasks_.empty()) return;
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    task();
                }
            });
        }
    }
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cond_.notify_all();
        for (auto &worker : workers_) worker.join();
    }

    template<typename F, typename... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }
        cond_.notify_one();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> stop_;
};