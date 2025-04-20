#pragma once

#include "build_config.hpp"
#include "compat_fixes.hpp"
#include "url_parser.hpp"
#include "monitoring.hpp"
#include "thread_pool.hpp"
#include "database.hpp"
#include "file_indexer.hpp"
#include "image_analyzer.hpp"
#include "content_analyzer.hpp"
#include "config.hpp"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <future>
#include <map>
#include <set>

// Forward declarations
class RequestHandler;
class ResponseHandler;

/**
 * @class WebCrawler
 * @brief Main crawler implementation
 */
class WebCrawler {
public:
    enum class CrawlerState {
        IDLE,
        RUNNING,
        PAUSED,
        STOPPING,
        STOPPED
    };
    
    struct CrawlerStats {
        int totalUrls;
        int visitedUrls;
        int queuedUrls;
        int failedUrls;
        int pendingUrls;
        int totalBytes;
        int imagesProcessed;
        int activeThreads;
    };
    
    /**
     * @brief Constructor
     * @param config Crawler configuration
     */
    WebCrawler(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~WebCrawler();
    
    /**
     * @brief Start the crawler
     * @param startUrl Starting URL
     * @return True if crawler started successfully
     */
    bool start(const std::string& startUrl = "");
    
    /**
     * @brief Stop the crawler
     */
    void stop();
    
    /**
     * @brief Pause the crawler
     */
    void pause();
    
    /**
     * @brief Resume the crawler
     */
    void resume();
    
    /**
     * @brief Get current crawler state
     * @return Current state
     */
    CrawlerState getState() const;
    
    /**
     * @brief Wait for crawler to finish
     * @param timeoutMs Timeout in milliseconds (0 for infinite)
     * @return True if crawler finished, false if timeout occurred
     */
    bool waitForCompletion(int timeoutMs = 0);
    
    /**
     * @brief Get current statistics
     * @return Current statistics
     */
    CrawlerStats getStats() const;
    
    /**
     * @brief Get progress percentage
     * @return Progress percentage (0-100)
     */
    int getProgressPercentage() const;
    
private:
    // Internal methods
    void crawlerThread();
    void scheduleUrl(const std::string& url, int depth);
    bool processUrl(const std::string& url, int depth);
    bool downloadPage(const std::string& url, std::string& content);
    void processImage(const std::string& url, const std::vector<uint8_t>& imageData);
    bool isImageUrl(const std::string& url);
    std::string getImageExtension(const std::string& url);
    
    // Configuration
    Config config;
    
    // State
    std::atomic<CrawlerState> state;
    std::atomic<int> activeThreads;
    std::atomic<int> failedRequests;
    
    // URL tracking
    struct UrlEntry {
        std::string url;
        int depth;
    };
    
    std::queue<UrlEntry> urlQueue;
    std::set<std::string> visitedUrls;
    std::set<std::string> pendingUrls;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    
    // Worker threads
    std::vector<std::thread> threads;
    
    // Components
    std::unique_ptr<ThreadPool> threadPool;
    std::unique_ptr<URLParser> urlParser;
    std::unique_ptr<Database> database;
    std::unique_ptr<FileIndexer> fileIndexer;
    std::unique_ptr<ImageAnalyzer> imageAnalyzer;
    std::unique_ptr<ContentAnalyzer> contentAnalyzer;
    std::unique_ptr<Monitoring> monitoring;
    
    // Statistics
    std::atomic<int> totalPages;
    std::atomic<int> totalBytes;
    std::atomic<int> imagesProcessed;
    
    // Helper methods
    std::string vectorToString(const std::vector<std::string>& vec);
}; 