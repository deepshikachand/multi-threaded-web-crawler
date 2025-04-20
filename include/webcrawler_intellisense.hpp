#pragma once

#ifdef __INTELLISENSE__

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <functional>

// Forward declarations of CURL types (to avoid including curl/curl.h)
typedef void CURL;
typedef long CURLcode;
typedef long curl_off_t;

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

/**
 * WebCrawler class - IntelliSense-only definition
 * This class contains all the member variables and method declarations 
 * to help IntelliSense recognize symbols in crawler.cpp
 */
class WebCrawler {
public:
    // Constructor and destructor
    WebCrawler(const std::string& startUrl, size_t maxThreads = 4, 
               int maxDepth = 3, const std::vector<std::string>& allowedDomains = {});
    ~WebCrawler();
    
    // Core API methods
    void start();
    void stop();
    
    // Statistics and monitoring
    size_t getPagesCrawled() const;
    size_t getQueueSize() const;
    size_t getActiveThreads() const;
    double getAverageResponseTime() const;
    size_t getTotalBytesDownloaded() const;
    size_t getFailedRequests() const;

    // Core functionality methods
    void crawlPage(const std::string& url, int depth);
    void processPage(const std::string& url, const std::string& content);
    void processImage(const std::string& url, const std::vector<uint8_t>& imageData);
    bool isImageUrl(const std::string& url);
    std::string getImageExtension(const std::string& url);
    void addNewUrls(const std::string& content, const std::string& baseUrl, int depth);
    
    // CURL-related methods
    CURL* getCurlHandle();
    void releaseCurlHandle(CURL* handle);
    void initializeCurlHandles();
    void cleanupCurlHandles();
    
    // CURL callback functions
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, 
                               curl_off_t ultotal, curl_off_t ulnow);

private:
    // Core component objects
    std::unique_ptr<ThreadPool> threadPool;
    std::unique_ptr<Database> database;
    std::unique_ptr<URLParser> urlParser;
    std::unique_ptr<FileIndexer> fileIndexer;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<CrawlerFeatures> crawlerFeatures;
    std::unique_ptr<Monitoring> monitoring;
    std::unique_ptr<ContentAnalyzer> contentAnalyzer;
    std::unique_ptr<ImageAnalyzer> imageAnalyzer;
    
    // Configuration
    std::string startUrl;
    size_t maxThreads;
    int maxDepth;
    std::vector<std::string> allowedDomains;
    
    // State variables
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
    
    // CURL related variables
    CURL* curl;
    std::vector<CURL*> curlHandles;
};

#endif // __INTELLISENSE__ 