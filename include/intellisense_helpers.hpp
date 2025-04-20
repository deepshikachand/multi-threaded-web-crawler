#pragma once

// This file is only for IntelliSense to recognize symbols in Visual Studio Code
// It is not included in the actual build

#ifndef INTELLISENSE_HELPERS_HPP
#define INTELLISENSE_HELPERS_HPP

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <thread>
#include <algorithm>
#include <regex>

// CURL types and functions
typedef void CURL;
typedef long CURLcode;
typedef long curl_off_t;

// CURL constants
#define CURLOPT_URL 1
#define CURLOPT_WRITEFUNCTION 2
#define CURLOPT_WRITEDATA 3
#define CURLOPT_XFERINFOFUNCTION 4
#define CURLOPT_XFERINFODATA 5
#define CURLOPT_HEADER 6
#define CURLOPT_NOBODY 7
#define CURLE_OK 0
#define CURLINFO_RESPONSE_CODE 100
#define CURLINFO_CONTENT_TYPE 101

// CURL functions
CURL* curl_easy_init();
void curl_easy_cleanup(CURL* handle);
CURLcode curl_easy_setopt(CURL* handle, int option, ...);
CURLcode curl_easy_perform(CURL* handle);
CURLcode curl_easy_getinfo(CURL* handle, int info, ...);
const char* curl_easy_strerror(CURLcode code);

// Forward declarations of all classes
class ThreadPool;
class URLParser;
class Database;
class FileIndexer;
class ResourceManager;
class CrawlerFeatures;
class Monitoring;
class ContentAnalyzer;
class ImageAnalyzer;
class WebCrawler;

// Classes used in the crawler
class ThreadPool {
public:
    ThreadPool(size_t numThreads) {}
    
    template<typename F>
    void enqueue(F&& f) { f(); }
    
    size_t getActiveThreadCount() const { return 0; }
};

class URLParser {
public:
    int getDepth(const std::string& url) const { return 0; }
    std::string getDomain(const std::string& url) const { return "example.com"; }
    std::string extractTitle(const std::string& content) const { return "Title"; }
    std::vector<std::string> extractUrls(const std::string& content) const { return {}; }
};

class Database {
public:
    Database(const std::string& dbPath) {}
    bool addPage(const std::string& url, const std::string& title) { return true; }
    bool addPage(const std::string& url, const std::string& title, const std::string& description, const std::string& content, int depth) { return true; }
    size_t getQueueSize() const { return 0; }
    bool addContentFeatures(const std::string& url, const std::string& features) { return true; }
    bool addImage(const std::string& url, const std::string& description, const std::vector<std::string>& labels, const std::vector<std::string>& objects) { return true; }
};

class FileIndexer {
public:
    FileIndexer(const std::string& basePath) {}
    bool savePage(const std::string& url, const std::string& content) { return true; }
    bool saveImage(const std::string& url, const std::vector<uint8_t>& imageData, const std::string& extension) { return true; }
};

class ResourceManager {
public:
    ResourceManager(size_t maxConnections, int rateLimitPerSecond) {}
    bool checkRateLimit(const std::string& domain) { return true; }
};

class CrawlerFeatures {
public:
    CrawlerFeatures() {}
    bool shouldFollowLink(const std::string& url, const std::string& referrer = "") { return true; }
    bool isSupportedContentType(const std::string& contentType) { return true; }
    std::string normalizeUrl(const std::string& url) { return url; }
};

class Monitoring {
public:
    enum class LogLevel { DEBUG, INFO, WARNING, ERROR };
    
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
               const std::string& metricsFile = "metrics.csv") {}
    ~Monitoring() {}
    
    void log(LogLevel level, const std::string& message) {}
    void setLogLevel(LogLevel level) {}
    void setLogFile(const std::string& filename) {}
    
    void recordMetric(const std::string& name, double value) {}
    std::vector<Metric> getMetrics(const std::string& name, std::chrono::minutes timeWindow) const { return {}; }
    
    void startProfiling(const std::string& operation) {}
    void stopProfiling(const std::string& operation) {}
    
    std::string levelToString(LogLevel level) const { return ""; }
    CrawlerStats getCurrentStats() const { return {}; }
};

class ContentAnalyzer {
public:
    struct ContentFeatures {
        std::string summary;
        std::string toString() const { return summary; }
    };
    
    ContentAnalyzer() {}
    ContentFeatures analyzeContent(const std::string& content) { return {}; }
};

class ImageAnalyzer {
public:
    struct ImageFeatures {
        bool isNSFW = false;
        std::string description;
        std::vector<std::string> labels;
        std::vector<std::string> objects;
    };
    
    ImageAnalyzer() {}
    ImageFeatures analyzeImageData(const std::vector<uint8_t>& imageData) { return {}; }
};

// Complete WebCrawler class definition for IntelliSense
class WebCrawler {
public:
    WebCrawler(const std::string& startUrl, size_t maxThreads = 4, 
               int maxDepth = 3, const std::vector<std::string>& allowedDomains = {});
    ~WebCrawler();
    
    // Core methods
    void start();
    void stop();
    
    // Get crawler statistics
    size_t getPagesCrawled() const { return pagesCrawled; }
    size_t getQueueSize() const;
    size_t getActiveThreads() const { return threadPool->getActiveThreadCount(); }
    double getAverageResponseTime() const { return averageResponseTime; }
    size_t getTotalBytesDownloaded() const { return totalBytesDownloaded; }
    size_t getFailedRequests() const { return failedRequests; }

    // Private member functions declared here for IntelliSense
    void crawlPage(const std::string& url, int depth);
    void processPage(const std::string& url, const std::string& content);
    void processImage(const std::string& url, const std::vector<uint8_t>& imageData);
    bool isImageUrl(const std::string& url);
    std::string getImageExtension(const std::string& url);
    void addNewUrls(const std::string& content, const std::string& baseUrl, int depth);
    CURL* getCurlHandle();
    void releaseCurlHandle(CURL* handle);
    void initializeCurlHandles();
    void cleanupCurlHandles();
    
    // Callback functions for CURL
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

private:
    // Core components
    std::unique_ptr<ThreadPool> threadPool;
    std::unique_ptr<Database> database;
    std::unique_ptr<URLParser> urlParser;
    std::unique_ptr<FileIndexer> fileIndexer;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<CrawlerFeatures> crawlerFeatures;
    std::unique_ptr<Monitoring> monitoring;
    std::unique_ptr<ContentAnalyzer> contentAnalyzer;
    std::unique_ptr<ImageAnalyzer> imageAnalyzer;
    
    // Crawler configuration
    std::string startUrl;
    size_t maxThreads;
    int maxDepth;
    std::vector<std::string> allowedDomains;
    
    // Crawler state
    std::atomic<bool> running;
    std::atomic<size_t> pagesCrawled;
    std::atomic<size_t> totalBytesDownloaded;
    std::atomic<size_t> failedRequests;
    std::atomic<double> averageResponseTime;
    
    // Synchronization
    std::mutex queueMutex;
    std::mutex metricsMutex;
    std::mutex curlMutex;
    std::vector<std::string> urlQueue;
    std::unordered_set<std::string> visitedUrls;
    
    // CURL handles for each thread
    CURL* curl;
    std::vector<CURL*> curlHandles;
};

#endif // INTELLISENSE_HELPERS_HPP 