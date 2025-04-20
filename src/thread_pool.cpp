#ifdef __INTELLISENSE__
#include "../include/thread_pool_intellisense.hpp"
#endif

#include "../include/thread_pool.hpp"
#include <stdexcept>
#include <iostream>

#ifdef __linux__
#include <sys/sysinfo.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

ThreadPool::ThreadPool(size_t numThreads) : stop(false), active_threads(0) {
    initializeThreadAttributes();
    
    // Resize thread attributes
    thread_attributes.resize(numThreads);
    
    // Create worker threads
    for (size_t i = 0; i < numThreads; ++i) {
        thread_attributes[i].thread_index = i;
        thread_attributes[i].processor_mask = 0;
        
        workers.emplace_back([this, i] {
            this->workerThread();
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    
    condition.notify_all();
    
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

#ifdef __linux__
    pthread_attr_destroy(&thread_attr);
#endif
}

void ThreadPool::initializeThreadAttributes() {
#ifndef _WIN32
    // This is only needed for non-Windows platforms
    pthread_attr_init(&thread_attr);
#endif
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            condition.wait(lock, [this] {
                return stop || !tasks.empty();
            });
            
            if (stop && tasks.empty()) {
                return;
            }
            
            active_threads++;
            task = tasks.top().func;
            tasks.pop();
        }
        
        // Execute the task
        task();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            active_threads--;
        }
    }
}

void ThreadPool::setThreadAffinity(size_t threadIndex, int cpuId) {
    if (threadIndex >= workers.size()) {
        return;
    }
    
#ifdef _WIN32
    // Windows implementation
    DWORD_PTR mask = (static_cast<DWORD_PTR>(1) << cpuId);
    SetThreadAffinityMask(workers[threadIndex].native_handle(), mask);
#else
    // Linux implementation
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuId, &cpuset);
    pthread_setaffinity_np(workers[threadIndex].native_handle(), sizeof(cpu_set_t), &cpuset);
#endif
    
    thread_attributes[threadIndex].processor_mask = 1ULL << cpuId;
}

size_t ThreadPool::getActiveThreadCount() const {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return active_threads;
}

// Template specialization implementation
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        
        tasks.push(Task{[task]() { (*task)(); }, 0});
    }
    
    condition.notify_one();
    return result;
}
