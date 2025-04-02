#include "resource_manager.hpp"
#include <stdexcept>
#include <algorithm>
#include <sys/statvfs.h>
#include <sys/resource.h>

ResourceManager::ResourceManager(size_t maxMemoryMB, size_t maxDiskGB)
    : currentMemoryUsage(0)
    , maxMemoryUsage(maxMemoryMB * 1024 * 1024)
    , currentDiskUsage(0)
    , maxDiskUsage(maxDiskGB * 1024 * 1024 * 1024) {
}

ResourceManager::~ResourceManager() {
    optimizeResources();
}

bool ResourceManager::allocateMemory(size_t size) {
    size_t newUsage = currentMemoryUsage + size;
    if (newUsage > maxMemoryUsage) {
        return false;
    }
    currentMemoryUsage = newUsage;
    return true;
}

void ResourceManager::releaseMemory(size_t size) {
    if (size > currentMemoryUsage) {
        currentMemoryUsage = 0;
    } else {
        currentMemoryUsage -= size;
    }
}

size_t ResourceManager::getCurrentMemoryUsage() const {
    return currentMemoryUsage;
}

size_t ResourceManager::getMaxMemoryUsage() const {
    return maxMemoryUsage;
}

bool ResourceManager::checkDiskSpace(const std::string& path, size_t requiredBytes) {
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0) {
        throw std::runtime_error("Failed to check disk space");
    }

    size_t availableSpace = stat.f_bsize * stat.f_bfree;
    return (availableSpace >= requiredBytes) && 
           (currentDiskUsage + requiredBytes <= maxDiskUsage);
}

void ResourceManager::updateDiskUsage(const std::string& path, size_t bytes) {
    currentDiskUsage += bytes;
}

size_t ResourceManager::getCurrentDiskUsage() const {
    return currentDiskUsage;
}

size_t ResourceManager::getMaxDiskUsage() const {
    return maxDiskUsage;
}

bool ResourceManager::checkRateLimit(const std::string& domain) {
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    auto now = std::chrono::steady_clock::now();
    
    auto& limit = rateLimits[domain];
    
    // Reset counter if window has passed
    if (now - limit.lastRequest > RATE_LIMIT_WINDOW) {
        limit.requestCount = 0;
        limit.lastRequest = now;
    }
    
    if (limit.requestCount >= MAX_REQUESTS_PER_MINUTE) {
        return false;
    }
    
    limit.requestCount++;
    return true;
}

void ResourceManager::updateRateLimit(const std::string& domain) {
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    auto& limit = rateLimits[domain];
    limit.lastRequest = std::chrono::steady_clock::now();
}

void ResourceManager::cleanupOldFiles(const std::string& directory, std::chrono::hours maxAge) {
    auto now = std::chrono::system_clock::now();
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            auto fileTime = std::filesystem::last_write_time(entry);
            auto fileTimePoint = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
            
            if (now - fileTimePoint > maxAge) {
                size_t fileSize = std::filesystem::file_size(entry);
                std::filesystem::remove(entry);
                currentDiskUsage -= fileSize;
            }
        }
    }
}

void ResourceManager::optimizeResources() {
    // Clean up old rate limit entries
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = rateLimits.begin(); it != rateLimits.end();) {
        if (now - it->second.lastRequest > RATE_LIMIT_WINDOW) {
            it = rateLimits.erase(it);
        } else {
            ++it;
        }
    }
} 