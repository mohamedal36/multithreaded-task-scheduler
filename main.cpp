#include <iostream>
#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <future>
#include <condition_variable>

class TaskScheduler
{
public:
    explicit TaskScheduler(std::size_t num_threads = std::thread::hardware_concurrency())
    {
        if (num_threads == 0)
            num_threads = 1;
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers_.emplace_back([this]()
                                  { worker_loop(); });
        }
    };
    ~TaskScheduler()
    {
        {
            std::unique_lock<std::mutex> locK(queue_mutex_);
            stop_.store(true);
        }
        cv_.notify_all();
        for (auto &w : workers_)
        {
            if (w.joinable())
                w.join();
        }
    };

    template <class F, class... Args>
    auto submit(F &&f, Args &&...args) -> std::future<typename std::invoke_result<F, Args...>::type>;

    // Deleted copy/move
    TaskScheduler(const TaskScheduler &) = delete;
    TaskScheduler &operator=(const TaskScheduler &other) = delete;

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> task_queue_;

    std::mutex queue_mutex_;
    // std::lock_guard<std::mutex> lock(queue_mutex_); // guard automatically or use unique_loc for more flexability
    std::atomic<bool> stop_{false}; // operations on it is atomic but take care don't use it in non atomic expressions => use for operations
    std::condition_variable cv_;
    void worker_loop()
    {
        while (true)
        {
            std::function<void()> task_;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_); // required for condition variables
                cv_.wait(lock, [this]()
                         { return stop_.load() || !task_queue_.empty(); });
                if (task_queue_.empty() && stop_.load())
                {
                    return;
                }
                task_ = std::move(task_queue_.front());
                task_queue_.pop();
            } // lock released
            task_();
        }
    };
};


int main()
{
    return 0;
}