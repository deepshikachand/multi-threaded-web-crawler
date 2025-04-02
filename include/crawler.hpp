#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <curl/curl.h>
#include "thread_pool.hpp"
#include "database.hpp"
#include "url_parser.hpp"
#include "file_indexer.hpp"
#include "resource_manager.hpp"
#include "crawler_features.hpp"
#include "monitoring.hpp"
#include "content_analyzer.hpp"
#include "image_analyzer.hpp"

class WebCrawler {
public:
    WebCrawler(const std::string& startUrl, size_t maxThreads = 4, 
               int maxDepth = 3, const std::vector<std::string>& allowedDomains = {});
    ~WebCrawler();
    
    // Start the crawling process
    void start();
    
    // Stop the crawling process
    void stop();
    
    // Get crawler statistics
    size_t getPagesCrawled() const { return pagesCrawled; }
    size_t getQueueSize() const { return urlQueue.size(); }
    size_t getActiveThreads() const { return threadPool->getActiveThreads(); }

    // Performance monitoring
    double getAverageResponseTime() const { return averageResponseTime; }
    size_t getTotalBytesDownloaded() const { return totalBytesDownloaded; }
    size_t getFailedRequests() const { return failedRequests; }

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
    std::vector<std::string> urlQueue;
    std::unordered_set<std::string> visitedUrls;
    
    // CURL handles for each thread
    CURL* curl;
    std::vector<CURL*> curlHandles;
    std::mutex curlMutex;
    
    // Worker function for crawling
    void crawlPage(const std::string& url, int depth);
    
    // Helper functions
    void processPage(const std::string& url, const std::string& content);
    void addNewUrls(const std::string& content, const std::string& baseUrl, int depth);
    CURL* getCurlHandle();
    void releaseCurlHandle(CURL* handle);
    void initializeCurlHandles();
    void cleanupCurlHandles();
    
    // Callback functions for CURL
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

    // Image handling
    void processImage(const std::string& url, const std::vector<uint8_t>& imageData);
    bool isImageUrl(const std::string& url);
    std::string getImageExtension(const std::string& url);
}; 