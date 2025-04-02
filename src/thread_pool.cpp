#include "thread_pool.hpp"
#include <stdexcept>
#include <sched.h>
#include <sys/sysinfo.h>

ThreadPool::ThreadPool(size_t numThreads) : stop(false), active_threads(0) {
    initializeThreadAttributes();
    
    // Get number of available CPUs
    int numCPUs = get_nprocs();
    
    for(size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);
        
        // Set thread affinity to specific CPU
        if (i < static_cast<size_t>(numCPUs)) {
            setThreadAffinity(i, i);
        }
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    
    for(std::thread &worker: workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    pthread_attr_destroy(&thread_attr);
}

void ThreadPool::initializeThreadAttributes() {
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&thread_attr, 1024 * 1024); // 1MB stack size
}

void ThreadPool::setThreadAffinity(size_t threadIndex, int cpuId) {
    if (threadIndex >= workers.size()) {
        return;
    }
    
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuId, &cpuset);
    
    pthread_t thread = workers[threadIndex].native_handle();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        throw std::runtime_error("Failed to set thread affinity");
    }
}

void ThreadPool::workerThread() {
    while(true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] {
                return stop || !tasks.empty();
            });
            
            if(stop && tasks.empty()) {
                return;
            }
            
            task = std::move(tasks.top());
            tasks.pop();
        }
        
        ++active_threads;
        task.func();
        --active_threads;
    }
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;
    using task_type = std::packaged_task<return_type()>;
    
    auto task = std::make_shared<task_type>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if(stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        
        tasks.push(Task{
            [task]() { (*task)(); },
            0  // Default priority
        });
    }
    
    condition.notify_one();
    return res;
}

size_t ThreadPool::getActiveThreadCount() const {
    return active_threads;
} 