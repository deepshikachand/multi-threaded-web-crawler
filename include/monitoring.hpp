#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iomanip>

class Monitoring {
public:
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    struct Metric {
        std::string name;
        double value;
        std::chrono::system_clock::time_point timestamp;
    };

    struct CrawlerStats {
        size_t pagesCrawled;
        size_t queueSize;
        size_t activeThreads;
        size_t failedRequests;
        size_t totalBytesDownloaded;
        double averageResponseTime;
        std::chrono::system_clock::time_point startTime;
    };

    Monitoring(const std::string& logFile = "crawler.log",
              const std::string& metricsFile = "metrics.csv");
    ~Monitoring();

    // Logging
    void log(LogLevel level, const std::string& message);
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);

    // Metrics
    void recordMetric(const std::string& name, double value);
    std::vector<Metric> getMetrics(const std::string& name,
                                 std::chrono::minutes timeWindow) const;
    void saveMetrics();
    void loadMetrics();

    // Statistics
    void updateStats(const CrawlerStats& stats);
    CrawlerStats getCurrentStats() const;
    void generateReport(const std::string& filename);

    // Performance monitoring
    void startProfiling(const std::string& operation);
    void stopProfiling(const std::string& operation);
    std::vector<std::pair<std::string, double>> getProfilingResults() const;

private:
    std::ofstream logStream;
    std::ofstream metricsStream;
    LogLevel currentLogLevel;
    std::mutex logMutex;
    std::mutex metricsMutex;
    
    std::vector<Metric> metrics;
    CrawlerStats currentStats;
    
    struct ProfilingEntry {
        std::chrono::steady_clock::time_point startTime;
        size_t callCount;
        double totalTime;
    };
    
    std::unordered_map<std::string, ProfilingEntry> profilingData;
    std::mutex profilingMutex;

    std::string getLogLevelString(LogLevel level) const;
    void writeMetricToFile(const Metric& metric);
    void cleanupOldMetrics();
}; 