#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <memory>
#include <filesystem>
#include <chrono>

class ResourceManager {
public:
    ResourceManager(size_t maxMemoryMB = 1024, size_t maxDiskGB = 10);
    ~ResourceManager();

    // Memory management
    bool allocateMemory(size_t size);
    void releaseMemory(size_t size);
    size_t getCurrentMemoryUsage() const;
    size_t getMaxMemoryUsage() const;

    // Disk space management
    bool checkDiskSpace(const std::string& path, size_t requiredBytes);
    void updateDiskUsage(const std::string& path, size_t bytes);
    size_t getCurrentDiskUsage() const;
    size_t getMaxDiskUsage() const;

    // Rate limiting
    bool checkRateLimit(const std::string& domain);
    void updateRateLimit(const std::string& domain);

    // Resource cleanup
    void cleanupOldFiles(const std::string& directory, std::chrono::hours maxAge);
    void optimizeResources();

private:
    struct DomainRateLimit {
        std::chrono::steady_clock::time_point lastRequest;
        int requestCount;
    };

    std::atomic<size_t> currentMemoryUsage;
    std::atomic<size_t> maxMemoryUsage;
    std::atomic<size_t> currentDiskUsage;
    std::atomic<size_t> maxDiskUsage;
    
    std::mutex rateLimitMutex;
    std::unordered_map<std::string, DomainRateLimit> rateLimits;
    
    static constexpr int MAX_REQUESTS_PER_MINUTE = 60;
    static constexpr std::chrono::minutes RATE_LIMIT_WINDOW{1};
}; 