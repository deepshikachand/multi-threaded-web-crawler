#include "resource_manager.hpp"
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#else
#include <sys/statvfs.h>
#include <sys/resource.h>
#endif

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
    size_t newUsage = currentMemoryUsage.load() + size;
    if (newUsage > maxMemoryUsage.load()) {
        return false;
    }
    currentMemoryUsage.store(newUsage);
    return true;
}

void ResourceManager::releaseMemory(size_t size) {
    size_t newUsage = currentMemoryUsage.load() - size;
    currentMemoryUsage.store(newUsage < 0 ? 0 : newUsage);
}

size_t ResourceManager::getCurrentMemoryUsage() const {
    return currentMemoryUsage.load();
}

size_t ResourceManager::getMaxMemoryUsage() const {
    return maxMemoryUsage.load();
}

bool ResourceManager::checkDiskSpace(const std::string& path, size_t requiredBytes) {
#ifdef _WIN32
    // Windows-specific disk space check
    ULARGE_INTEGER freeBytesAvailable;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;

    if (!GetDiskFreeSpaceEx(path.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        throw std::runtime_error("Failed to check disk space on Windows");
    }

    size_t availableSpace = freeBytesAvailable.QuadPart;
#else
    // Linux-specific disk space check
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0) {
        throw std::runtime_error("Failed to check disk space");
    }

    size_t availableSpace = stat.f_bsize * stat.f_bfree;
#endif

    return (availableSpace >= requiredBytes) && (currentDiskUsage.load() + requiredBytes <= maxDiskUsage.load());
}

void ResourceManager::updateDiskUsage(const std::string& path, size_t bytes) {
    currentDiskUsage.fetch_add(bytes);
}

size_t ResourceManager::getCurrentDiskUsage() const {
    return currentDiskUsage.load();
}

size_t ResourceManager::getMaxDiskUsage() const {
    return maxDiskUsage.load();
}

bool ResourceManager::checkRateLimit(const std::string& domain) {
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    auto now = std::chrono::steady_clock::now();
    
    auto& limit = rateLimits[domain];
    
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
            
            // Convert filesystem time to system_clock time
            auto fileTimeDuration = fileTime.time_since_epoch();
            auto fileTimePoint = std::chrono::system_clock::time_point(fileTimeDuration);
            
            if (now - fileTimePoint > maxAge) {
                size_t fileSize = std::filesystem::file_size(entry);
                std::filesystem::remove(entry);
                currentDiskUsage.fetch_sub(fileSize);
            }
        }
    }
}

void ResourceManager::optimizeResources() {
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
