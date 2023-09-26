#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

namespace lod
{
    // A simple task queue for each thread
    class TaskQueue
    {
    private:
        std::deque<std::function<void()>> tasks;
        std::mutex mutex;
        std::condition_variable cv;

    public:
        void push_back(std::function<void()> task)
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.push_back(std::move(task));
            cv.notify_one();
        }

        void push_front(std::function<void()> task)
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.push_front(std::move(task));
            cv.notify_one();
        }

        std::function<void()> pop_front()
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (tasks.empty())
                return {};
            auto task = std::move(tasks.front());
            tasks.pop_front();
            return task;
        }

        std::function<void()> steal()
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (tasks.empty())
                return {};
            auto task = std::move(tasks.back());
            tasks.pop_back();
            return task;
        }
    };

    // A thread pool with work stealing
    class ThreadPool
    {
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;

        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;

        void workerLoop()
        {
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this]
                                   { return stop || !tasks.empty(); });
                    if (stop && tasks.empty())
                        return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        }

    public:
        ThreadPool(size_t threads) : stop(false)
        {
            for (size_t i = 0; i < threads; ++i)
                workers.emplace_back(&ThreadPool::workerLoop, this);
        }
        ~ThreadPool()
        {
            stopAndJoin();
        }

        template <class F>
        void enqueue(F &&f)
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                tasks.emplace(std::forward<F>(f));
            }
            condition.notify_one();
        }

        void stopAndJoin()
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }
            condition.notify_all();
            for (std::thread &worker : workers)
                worker.join();
            workers.clear();
            tasks = std::queue<std::function<void()>>();
        }
        void restart(size_t threads)
        {
            stopAndJoin();

            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop && workers.empty())
            {
                stop = false;
                for (size_t i = 0; i < threads; ++i)
                    workers.emplace_back(&ThreadPool::workerLoop, this);
            }
        }
    };
}
