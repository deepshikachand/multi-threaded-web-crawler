#include "monitoring.hpp"
#include <stdexcept>
#include <algorithm>
#include <filesystem>

Monitoring::Monitoring(const std::string& logFile, const std::string& metricsFile)
    : currentLogLevel(LogLevel::INFO) {
    logStream.open(logFile, std::ios::app);
    metricsStream.open(metricsFile, std::ios::app);
    
    if (!logStream.is_open()) {
        throw std::runtime_error("Failed to open log file: " + logFile);
    }
    if (!metricsStream.is_open()) {
        throw std::runtime_error("Failed to open metrics file: " + metricsFile);
    }

    // Initialize stats
    currentStats = CrawlerStats{
        .pagesCrawled = 0,
        .queueSize = 0,
        .activeThreads = 0,
        .failedRequests = 0,
        .totalBytesDownloaded = 0,
        .averageResponseTime = 0.0,
        .startTime = std::chrono::system_clock::now()
    };
}

Monitoring::~Monitoring() {
    if (logStream.is_open()) {
        logStream.close();
    }
    if (metricsStream.is_open()) {
        metricsStream.close();
    }
}

void Monitoring::log(LogLevel level, const std::string& message) {
    if (level < currentLogLevel) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    
    logStream << "[" << ss.str() << "] [" 
              << getLogLevelString(level) << "] "
              << message << std::endl;
}

void Monitoring::setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

void Monitoring::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logStream.is_open()) {
        logStream.close();
    }
    logStream.open(filename, std::ios::app);
    if (!logStream.is_open()) {
        throw std::runtime_error("Failed to open new log file: " + filename);
    }
}

void Monitoring::recordMetric(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    Metric metric{
        .name = name,
        .value = value,
        .timestamp = std::chrono::system_clock::now()
    };
    
    metrics.push_back(metric);
    writeMetricToFile(metric);
    cleanupOldMetrics();
}

std::vector<Monitoring::Metric> Monitoring::getMetrics(
    const std::string& name,
    std::chrono::minutes timeWindow) const {
    
    std::lock_guard<std::mutex> lock(metricsMutex);
    auto now = std::chrono::system_clock::now();
    
    std::vector<Metric> filtered;
    std::copy_if(metrics.begin(), metrics.end(),
                 std::back_inserter(filtered),
                 [&](const Metric& m) {
                     return m.name == name &&
                            (now - m.timestamp) <= timeWindow;
                 });
    
    return filtered;
}

void Monitoring::saveMetrics() {
    std::lock_guard<std::mutex> lock(metricsMutex);
    for (const auto& metric : metrics) {
        writeMetricToFile(metric);
    }
}

void Monitoring::loadMetrics() {
    std::lock_guard<std::mutex> lock(metricsMutex);
    metrics.clear();
    
    std::ifstream file(metricsStream.filename());
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name, timestamp;
        double value;
        
        if (std::getline(ss, name, ',') &&
            ss >> value &&
            std::getline(ss, timestamp)) {
            
            Metric metric{
                .name = name,
                .value = value,
                .timestamp = std::chrono::system_clock::from_time_t(
                    std::stoll(timestamp))
            };
            metrics.push_back(metric);
        }
    }
}

void Monitoring::updateStats(const CrawlerStats& stats) {
    currentStats = stats;
}

Monitoring::CrawlerStats Monitoring::getCurrentStats() const {
    return currentStats;
}

void Monitoring::generateReport(const std::string& filename) {
    std::ofstream report(filename);
    if (!report.is_open()) {
        throw std::runtime_error("Failed to create report file: " + filename);
    }

    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        now - currentStats.startTime).count();

    report << "Crawler Report\n"
           << "=============\n\n"
           << "Duration: " << duration << " seconds\n"
           << "Pages Crawled: " << currentStats.pagesCrawled << "\n"
           << "Queue Size: " << currentStats.queueSize << "\n"
           << "Active Threads: " << currentStats.activeThreads << "\n"
           << "Failed Requests: " << currentStats.failedRequests << "\n"
           << "Total Downloaded: " << (currentStats.totalBytesDownloaded / 1024 / 1024)
           << " MB\n"
           << "Average Response Time: " << std::fixed << std::setprecision(2)
           << currentStats.averageResponseTime << "s\n\n"
           << "Performance Metrics\n"
           << "==================\n";

    auto profilingResults = getProfilingResults();
    for (const auto& [operation, time] : profilingResults) {
        report << operation << ": " << time << "s\n";
    }
}

void Monitoring::startProfiling(const std::string& operation) {
    std::lock_guard<std::mutex> lock(profilingMutex);
    auto& entry = profilingData[operation];
    entry.startTime = std::chrono::steady_clock::now();
    entry.callCount++;
}

void Monitoring::stopProfiling(const std::string& operation) {
    std::lock_guard<std::mutex> lock(profilingMutex);
    auto it = profilingData.find(operation);
    if (it != profilingData.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - it->second.startTime).count();
        it->second.totalTime += duration / 1000000.0;
    }
}

std::vector<std::pair<std::string, double>> Monitoring::getProfilingResults() const {
    std::lock_guard<std::mutex> lock(profilingMutex);
    std::vector<std::pair<std::string, double>> results;
    
    for (const auto& [operation, entry] : profilingData) {
        if (entry.callCount > 0) {
            results.emplace_back(operation, entry.totalTime / entry.callCount);
        }
    }
    
    return results;
}

std::string Monitoring::getLogLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void Monitoring::writeMetricToFile(const Metric& metric) {
    auto timestamp = std::chrono::system_clock::to_time_t(metric.timestamp);
    metricsStream << metric.name << "," << metric.value << "," 
                 << timestamp << std::endl;
}

void Monitoring::cleanupOldMetrics() {
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::hours(24); // Keep last 24 hours
    
    metrics.erase(
        std::remove_if(metrics.begin(), metrics.end(),
            [&](const Metric& m) { return m.timestamp < cutoff; }),
        metrics.end()
    );
} 