//#include <thread>
//#include <iostream>
//#include <mutex>
//#include <condition_variable>
//#include <future>
//#include <vector>
//#include <queue>
//#include <ranges>
//#include <algorithm>
//#include <chrono>
//
////#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
//
//class ThreadPool {
//public:
//    explicit ThreadPool(size_t num_threads) {
//        workers_.reserve(num_threads);
//        for ([[maybe_unused]] size_t _ : std::views::iota(0u, num_threads)) {
//            workers_.emplace_back(&ThreadPool::Worker, this);
//        }
//    }
//
//    ~ThreadPool() {
//        stop_ = true;
//        cv_.notify_all();
//        std::ranges::for_each(workers_, [](std::thread& t) { 
//            if (t.joinable()) {
//                t.join();
//            }
//        });
//    }
//
//    template <class F>
//    void Enqueue(F&& task) {
//        {
//            std::lock_guard lock(queue_mutex_);
//            tasks_.emplace(std::forward<F>(task));
//        }
//        cv_.notify_one();
//    }
//
//    void Stop() {
//        stop_ = true;
//        cv_.notify_all();
//    }    
//
//private:
//    std::mutex queue_mutex_;
//    std::condition_variable cv_;
//    std::atomic<bool> stop_ = false;
//
//    std::vector<std::thread> workers_;
//    std::queue<std::function<void()>> tasks_;
//
//    void Worker() {
//        while (true) {
//            //std::this_thread::yield();
//            std::function<void()> task;
//            {
//                std::unique_lock lock(queue_mutex_);
//                cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
//
//                if (stop_ || tasks_.empty()) return;
//
//                task = std::move(tasks_.front());
//                tasks_.pop();
//            }
//            task();
//        }
//    }
//};
//
//int main() {
//    auto tp = std::make_unique<ThreadPool>(8);
//    std::mutex output_mutex;
//    const auto finite_task = [&output_mutex](int id) {
//        for (int i = 0; i < 3; ++i) {
//            {
//                std::lock_guard lock(output_mutex);
//                std::cout << "Task " << id << " step " << i + 1 << std::endl;
//            }
//            std::this_thread::sleep_for(std::chrono::milliseconds(100));
//        }
//    };
//
//    for (int i = 0; i < 10; ++i) {
//        tp->Enqueue([i, finite_task] { finite_task(i); });
//    }
//
//	std::this_thread::sleep_for(std::chrono::seconds(5));
//    //tp.Stop();
//    return 0;
//}