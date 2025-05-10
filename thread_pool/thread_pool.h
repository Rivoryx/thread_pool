#pragma once

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <ranges>
#include <thread>
#include <vector>
#include <iostream>

//#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        workers_.reserve(num_threads);
        for ([[maybe_unused]] size_t _ : std::views::iota(0u, num_threads)) {
            workers_.emplace_back(&ThreadPool::Worker, this);
        }
    }

    ~ThreadPool() {
        Stop();
        Wait();
        std::ranges::for_each(workers_, [](std::thread& t) { 
            if (t.joinable()) {
                t.join();
            }
        });
    }

    template <class F>
    void Enqueue(F&& task) {
        {
            std::lock_guard lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(task));
        }
        cv_.notify_one();
    }

    void Stop() {
        stop_ = true;
        cv_.notify_all();
    }

    void Wait() {
        std::unique_lock lock(queue_mutex_);
        // Waining for ALL tasks to finish (in queue + already running)
        cv_finished_.wait(lock, [this]() {
            return tasks_.empty() && (active_tasks_ == 0);
        });
    }

private:
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::condition_variable cv_finished_;
    std::atomic<bool> stop_ = false;
    std::atomic<size_t> active_tasks_ = 0;

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    void Worker() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(queue_mutex_);
                cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });

                if (stop_ && tasks_.empty()) {
                    cv_finished_.notify_all();
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
                active_tasks_++;
            }

            try {
                task();
            }
            catch (std::filesystem::filesystem_error& e) {
                --active_tasks_;
                throw e;
            }
            catch(...) {
                --active_tasks_;
                std::cerr << "Unknown error in thread pool worker" << std::endl;
                throw;
            }

            {
                std::lock_guard lock(queue_mutex_);
                active_tasks_--;
                if (tasks_.empty() && active_tasks_ == 0) {
                    cv_finished_.notify_all();
                }
            }
        }
    }
};