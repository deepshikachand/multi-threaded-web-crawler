#pragma once

#ifdef __INTELLISENSE__

#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <mutex>
#include <map>
#include <fstream>

class Monitoring {
public:
    /**
     * @enum LogLevel
     * @brief Defines logging severity levels
     */
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        LOG_ERROR,
        CRITICAL
    };

    /**
     * @struct Metrics
     * @brief Contains performance metrics
     */
    struct Metrics {
        int pagesCrawled;
        int failedRequests;
        int imagesProcessed;
        int urlsQueued;
        int activeThreads;
    };

    /**
     * @struct ProfilingData
     * @brief Contains timing data for performance profiling
     */
    struct ProfilingData {
        std::chrono::duration<double> totalTime;
        int callCount;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    };

    /**
     * @brief Constructor
     * @param logFilePath Path to the log file
     * @param level Initial log level
     */
    Monitoring(const std::string& logFilePath, LogLevel level = LogLevel::INFO);
    
    /**
     * @brief Destructor
     */
    ~Monitoring();

    /**
     * @brief Log a message
     * @param level Log level
     * @param message Message to log
     */
    void log(LogLevel level, const std::string& message);
    
    /**
     * @brief Log a message with a format string
     * @param level Log level
     * @param format Format string
     * @param ... Format arguments
     */
    void logf(LogLevel level, const char* format, ...);

    /**
     * @brief Get current metrics
     * @return Current metrics
     */
    Metrics getMetrics() const;

    /**
     * @brief Update metrics
     * @param metrics New metrics
     */
    void updateMetrics(const Metrics& metrics);

    /**
     * @brief Get current stats as a string
     * @return Stats string
     */
    std::string getCurrentStats() const;

    /**
     * @brief Start profiling an operation
     * @param operationName Name of the operation
     */
    void startProfiling(const std::string& operationName);

    /**
     * @brief Stop profiling an operation
     * @param operationName Name of the operation
     */
    void stopProfiling(const std::string& operationName);

    /**
     * @brief Get profiling results
     * @return Map of operation names to profiling data
     */
    std::map<std::string, ProfilingData> getProfilingResults() const;

    /**
     * @brief Get average time for an operation
     * @param operationName Name of the operation
     * @return Average time in seconds
     */
    double getAverageOperationTime(const std::string& operationName) const;

    /**
     * @brief Convert log level to string
     * @param level Log level
     * @return String representation of log level
     */
    std::string logLevelToString(LogLevel level) const;
};

#endif // __INTELLISENSE__ 