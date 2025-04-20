#include "../include/monitoring.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>
#include <cstdarg>
#include <cstring>

Monitoring::Monitoring(const std::string& logFilePath, LogLevel level)
    : currentLogLevel(level), logFilePath(logFilePath) {
    
    // Initialize metrics
    metrics.pagesCrawled = 0;
    metrics.failedRequests = 0;
    metrics.imagesProcessed = 0;
    metrics.urlsQueued = 0;
    metrics.activeThreads = 0;
    
    // Open log file
    logFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
    }
    
    // Log initialization
    log(LogLevel::INFO, "Monitoring system initialized");
}

Monitoring::~Monitoring() {
    // Log shutdown
    log(LogLevel::INFO, "Monitoring system shutdown");
    
    // Close log file
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Monitoring::log(LogLevel level, const std::string& message) {
    // Skip if level is below current log level
    if (level < currentLogLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &timeT);
#else
    localtime_r(&timeT, &timeInfo);
#endif
    
    // Format time string
    std::ostringstream timeStr;
    timeStr << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    
    // Format log line
    std::string logLine = timeStr.str() + " [" + logLevelToString(level) + "] " + message;
    
    // Output to console and file
    std::cout << logLine << std::endl;
    
    if (logFile.is_open()) {
        logFile << logLine << std::endl;
        logFile.flush();
    }
}

void Monitoring::logf(LogLevel level, const char* format, ...) {
    // Skip if level is below current log level
    if (level < currentLogLevel) {
        return;
    }
    
    // Format the message using varargs
    va_list args;
    va_start(args, format);
    
    // Determine required buffer size
    va_list argsCopy;
    va_copy(argsCopy, args);
    const int bufferSize = vsnprintf(nullptr, 0, format, argsCopy) + 1;
    va_end(argsCopy);
    
    if (bufferSize <= 0) {
        va_end(args);
        log(level, "Error formatting log message");
        return;
    }
    
    // Format the message
    std::vector<char> buffer(bufferSize);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);
    
    // Log the formatted message
    log(level, std::string(buffer.data()));
}

Monitoring::Metrics Monitoring::getMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    return metrics;
}

void Monitoring::updateMetrics(const Metrics& newMetrics) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    metrics = newMetrics;
}

std::string Monitoring::getCurrentStats() const {
    Metrics current = getMetrics();
    
    std::ostringstream stats;
    stats << "Pages crawled: " << current.pagesCrawled
          << ", Failed requests: " << current.failedRequests
          << ", Images processed: " << current.imagesProcessed
          << ", URLs queued: " << current.urlsQueued
          << ", Active threads: " << current.activeThreads;
    
    return stats.str();
}

void Monitoring::startProfiling(const std::string& operationName) {
    std::lock_guard<std::mutex> lock(profilingMutex);
    
    // Create entry if it doesn't exist
    if (profilingData.find(operationName) == profilingData.end()) {
        ProfilingData data;
        data.totalTime = std::chrono::duration<double>(0);
        data.callCount = 0;
        profilingData[operationName] = data;
    }
    
    // Set start time
    profilingData[operationName].startTime = std::chrono::high_resolution_clock::now();
}

void Monitoring::stopProfiling(const std::string& operationName) {
    std::lock_guard<std::mutex> lock(profilingMutex);
    
    auto it = profilingData.find(operationName);
    if (it != profilingData.end()) {
        // Calculate elapsed time
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsed = endTime - it->second.startTime;
        
        // Update profiling data
        it->second.totalTime += elapsed;
        it->second.callCount++;
    }
}

std::map<std::string, Monitoring::ProfilingData> Monitoring::getProfilingResults() const {
    std::lock_guard<std::mutex> lock(profilingMutex);
    return profilingData;
}

double Monitoring::getAverageOperationTime(const std::string& operationName) const {
    std::lock_guard<std::mutex> lock(profilingMutex);
    
    auto it = profilingData.find(operationName);
    if (it != profilingData.end() && it->second.callCount > 0) {
        return it->second.totalTime.count() / it->second.callCount;
    }
    
    return 0.0;
}

std::string Monitoring::logLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::LOG_ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}
