#pragma once

#include "build_config.hpp"
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

// Define ThreadPoolAttributes struct
struct ThreadPoolAttributes {
    size_t thread_index;
    unsigned long long processor_mask;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Add a task to the thread pool
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    // Get the number of active threads
    size_t getActiveThreadCount() const;

    // Set thread affinity for better performance
    void setThreadAffinity(size_t threadIndex, int cpuId);

private:
    // Worker threads
    std::vector<std::thread> workers;

    // Thread attributes
    std::vector<ThreadPoolAttributes> thread_attributes;

    // Task queue with priority
    struct Task {
        std::function<void()> func;
        int priority;
        bool operator<(const Task& other) const {
            return priority < other.priority;
        }
    };
    std::priority_queue<Task> tasks;

    // Synchronization primitives
    mutable std::mutex queue_mutex;
    std::condition_variable condition;

    // Thread pool state
    std::atomic<bool> stop;
    std::atomic<size_t> active_threads;

#ifndef _WIN32
    // Thread attributes for Linux
    pthread_attr_t thread_attr;
#endif

    // Helper functions
    void workerThread();
    void initializeThreadAttributes();
};
