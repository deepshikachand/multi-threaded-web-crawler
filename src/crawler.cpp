#ifdef __INTELLISENSE__
#include "../include/webcrawler_class.hpp"
#include "../include/intellisense_helpers.hpp"
#include "../include/database_extensions.hpp"
#include "../include/monitoring_extensions.hpp"
#endif

#include "../include/crawler.hpp"
#include "../include/compat_fixes.hpp"
#include <stdexcept>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <regex>
#include <thread>
#include <mutex>
#include <algorithm>
#include <iostream>
#include <fstream>

#ifdef STUB_IMPLEMENTATION
#include "curl_stubs.hpp"

// Stub classes for missing dependencies
class ThreadPool {
public:
    ThreadPool(size_t) {}
    template<typename F>
    void enqueue(F&& f) { f(); }
    size_t getActiveThreadCount() const { return 0; }
};

class URLParser {
public:
    int getDepth(const std::string&) const { return 0; }
    std::string getDomain(const std::string&) const { return "example.com"; }
    std::string extractTitle(const std::string&) const { return "Title"; }
    std::vector<std::string> extractLinks(const std::string&, const std::string&) const { return {}; }
    std::vector<std::string> extractImages(const std::string&, const std::string&) const { return {}; }
};

class Database {
public:
    Database(const std::string&) {}
    bool addPage(const std::string&, const std::string&) { return true; }
    bool addPage(const std::string&, const std::string&, const std::string&, const std::string&) { return true; }
    size_t getQueueSize() const { return 0; }
    bool addContentFeatures(const std::string&, const std::string&) { return true; }
    bool addImage(const std::string&, const std::string&, const std::string&, const std::string&) { return true; }
};

class FileIndexer {
public:
    FileIndexer(const std::string&) {}
    bool savePage(const std::string&, const std::string&) { return true; }
    std::string getPagePath(const std::string&) { return "page_path.html"; }

    // Define saveImage method
    bool saveImage(const std::string& url, const std::vector<uint8_t>& imageData, const std::string& extension) {
        // Save image logic here
        return true;
    }
};

class ResourceManager {
public:
    ResourceManager(size_t, int) {}
    bool checkRateLimit(const std::string&) { return true; }
};

class CrawlerFeatures {
public:
    CrawlerFeatures() {}
    bool shouldFollowLink(const std::string&, const std::string&) const { return true; }
    bool isSupportedContentType(const std::string&) const { return true; }
    std::string normalizeUrl(const std::string&) const { return "normalized_url"; }
};

class Monitoring {
public:
    enum class LogLevel { DEBUG, INFO, WARNING, LOG_ERROR, CRITICAL };
    
    Monitoring(const std::string& logFile = "crawler.log", 
               const std::string& metricsFile = "metrics.csv") {}
    
    void log(LogLevel level, const std::string& message) {
        // In a stub implementation, we could just print to console
        std::string levelStr;
        switch (level) {
            case LogLevel::DEBUG: levelStr = "DEBUG"; break;
            case LogLevel::INFO: levelStr = "INFO"; break;
            case LogLevel::WARNING: levelStr = "WARNING"; break;
            case LogLevel::LOG_ERROR: levelStr = "ERROR"; break;
            case LogLevel::CRITICAL: levelStr = "CRITICAL"; break;
        }
        std::cout << "[" << levelStr << "] " << message << std::endl;
    }
    
    void startProfiling(const std::string& operation) {}
    void stopProfiling(const std::string& operation) {}
    
    std::string logLevelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::LOG_ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};

class ContentAnalyzer {
public:
    ContentAnalyzer() {}
    
    struct ContentFeatures {
        std::string toString() const { return ""; }
    };
    
    ContentFeatures analyzeContent(const std::string&) const { return {}; }
};

class ImageAnalyzer {
public:
    ImageAnalyzer() {}
    
    struct ImageFeatures {
        bool isNSFW = false;
        std::string description;
        std::vector<std::string> labels;
        std::vector<std::string> objects;
    };
    
    ImageFeatures analyzeImageData(const std::vector<uint8_t>&) const { return {}; }
};

#else

// Include real implementations here

#endif

WebCrawler::WebCrawler(const Config& config)
    : config(config)
    , state(CrawlerState::IDLE)
    , activeThreads(0)
    , failedRequests(0)
    , totalPages(0)
    , totalBytes(0)
    , imagesProcessed(0) {
    
    // Initialize components
    threadPool = std::make_unique<ThreadPool>(config.getThreadCount());
    urlParser = std::make_unique<URLParser>();
    database = std::make_unique<Database>(config.getDatabasePath());
    fileIndexer = std::make_unique<FileIndexer>(config.getContentDirectory());
    imageAnalyzer = std::make_unique<ImageAnalyzer>();
    contentAnalyzer = std::make_unique<ContentAnalyzer>();
    monitoring = std::make_unique<Monitoring>(config.getLogFilePath());
    
    // Log initialization
    monitoring->log(Monitoring::LogLevel::INFO, "WebCrawler initialized");
}

WebCrawler::~WebCrawler() {
    // Stop crawler if still running
    if (state == CrawlerState::RUNNING || state == CrawlerState::PAUSED) {
        stop();
    }
    
    // Log shutdown
    monitoring->log(Monitoring::LogLevel::INFO, "WebCrawler destroyed");
}

bool WebCrawler::start(const std::string& startUrl) {
    // Check if already running
    if (state == CrawlerState::RUNNING) {
        monitoring->log(Monitoring::LogLevel::WARNING, "Crawler is already running");
        return false;
    }
    
    // Use provided startUrl or default from config
    std::string urlToStart = startUrl.empty() ? config.getStartUrl() : startUrl;
    
    monitoring->log(Monitoring::LogLevel::INFO, "Starting crawler with URL: " + urlToStart);
    
    // Reset state and counters
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        urlQueue = std::queue<UrlEntry>();
        visitedUrls.clear();
        pendingUrls.clear();
        activeThreads = 0;
        failedRequests = 0;
        totalPages = 0;
        totalBytes = 0;
        imagesProcessed = 0;
    }
    
    // Add start URL to queue
    scheduleUrl(urlToStart, 0);
    
    // Update state
    state = CrawlerState::RUNNING;
    
    // Start worker threads
    int numThreads = config.getThreadCount();
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(&WebCrawler::crawlerThread, this);
    }
    
    return true;
}

void WebCrawler::stop() {
    if (state != CrawlerState::RUNNING && state != CrawlerState::PAUSED) {
        return;
    }
    
    monitoring->log(Monitoring::LogLevel::INFO, "Stopping crawler");
    
    // Set state to stopping
    state = CrawlerState::STOPPING;
    
    // Notify all waiting threads
    queueCondition.notify_all();
    
    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Clear threads vector
    threads.clear();
    
    // Update state
    state = CrawlerState::STOPPED;
    
    monitoring->log(Monitoring::LogLevel::INFO, "Crawler stopped");
}

void WebCrawler::pause() {
    if (state != CrawlerState::RUNNING) {
        return;
    }
    
    monitoring->log(Monitoring::LogLevel::INFO, "Pausing crawler");
    state = CrawlerState::PAUSED;
}

void WebCrawler::resume() {
    if (state != CrawlerState::PAUSED) {
        return;
    }
    
    monitoring->log(Monitoring::LogLevel::INFO, "Resuming crawler");
    state = CrawlerState::RUNNING;
    
    // Notify all waiting threads
    queueCondition.notify_all();
}

WebCrawler::CrawlerState WebCrawler::getState() const {
    return state;
}

bool WebCrawler::waitForCompletion(int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();
    
    while ((state == CrawlerState::RUNNING || state == CrawlerState::PAUSED) &&
           (timeoutMs == 0 || 
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count() < timeoutMs)) {
        
        // Check if queue is empty and no active threads
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (urlQueue.empty() && activeThreads == 0) {
                // Crawler finished
                state = CrawlerState::STOPPED;
                return true;
            }
        }
        
        // Sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return state == CrawlerState::STOPPED;
}

WebCrawler::CrawlerStats WebCrawler::getStats() const {
    CrawlerStats stats;
    stats.totalUrls = static_cast<int>(visitedUrls.size() + pendingUrls.size());
    stats.visitedUrls = static_cast<int>(visitedUrls.size());
    stats.queuedUrls = static_cast<int>(urlQueue.size());
    stats.failedUrls = failedRequests;
    stats.pendingUrls = static_cast<int>(pendingUrls.size());
    stats.totalBytes = totalBytes;
    stats.imagesProcessed = imagesProcessed;
    stats.activeThreads = activeThreads;
    return stats;
}

int WebCrawler::getProgressPercentage() const {
    int maxPages = config.getMaxPages();
    if (maxPages <= 0) {
        return 0;
    }
    
    int percentage = static_cast<int>((static_cast<int>(visitedUrls.size()) * 100) / maxPages);
    return percentage > 100 ? 100 : percentage;
}

void WebCrawler::crawlerThread() {
    activeThreads++;
    
    while (state == CrawlerState::RUNNING || state == CrawlerState::PAUSED) {
        // Wait if paused
        if (state == CrawlerState::PAUSED) {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this] {
                return state != CrawlerState::PAUSED || state == CrawlerState::STOPPING;
            });
            continue;
        }
        
        // Get URL from queue
        UrlEntry entry;
        bool hasUrl = false;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (urlQueue.empty()) {
                // No URLs to process, wait for new ones or timeout
                queueCondition.wait_for(lock, std::chrono::milliseconds(1000), [this] {
                    return !urlQueue.empty() || state != CrawlerState::RUNNING;
                });
                
                if (urlQueue.empty()) {
                    // Still no URLs after waiting, check if we should exit
                    if (pendingUrls.empty() && state == CrawlerState::RUNNING) {
                        // No URLs in queue or pending, and crawler is running
                        // This indicates that crawling is done
                        state = CrawlerState::STOPPED;
                    }
                    continue;
                }
            }
            
            // Get URL from queue
            entry = urlQueue.front();
            urlQueue.pop();
            
            // Mark as pending
            pendingUrls.insert(entry.url);
            hasUrl = true;
        }
        
        if (hasUrl) {
            // Process URL
            processUrl(entry.url, entry.depth);
            
            // Mark as visited and remove from pending
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                visitedUrls.insert(entry.url);
                pendingUrls.erase(entry.url);
            }
        }
    }
    
    activeThreads--;
}

void WebCrawler::scheduleUrl(const std::string& url, int depth) {
    std::lock_guard<std::mutex> lock(queueMutex);
    
    // Skip if URL has already been visited or is pending
    if (visitedUrls.find(url) != visitedUrls.end() || pendingUrls.find(url) != pendingUrls.end()) {
        return;
    }
    
    // Skip if depth exceeds max depth
    if (depth > config.getMaxDepth()) {
        return;
    }
    
    // Skip if reached max pages
    int maxPages = config.getMaxPages();
    if (maxPages > 0 && (static_cast<int>(visitedUrls.size()) + static_cast<int>(pendingUrls.size())) >= maxPages) {
        return;
    }
    
    // Add URL to queue
    UrlEntry entry;
    entry.url = url;
    entry.depth = depth;
    urlQueue.push(entry);
    
    // Notify waiting thread
    queueCondition.notify_one();
}

bool WebCrawler::processUrl(const std::string& url, int depth) {
    monitoring->log(Monitoring::LogLevel::INFO, "Processing URL: " + url + " (depth: " + std::to_string(depth) + ")");
    
    // Download page content
    std::string content;
    if (!downloadPage(url, content)) {
        failedRequests++;
        monitoring->log(Monitoring::LogLevel::LOG_ERROR, "Failed to download: " + url);
        return false;
    }
    
    // Update total bytes downloaded
    totalBytes += content.size();
    
    // Check if it's an image
    if (isImageUrl(url)) {
        // Process as image
        std::vector<uint8_t> imageData(content.begin(), content.end());
        processImage(url, imageData);
        return true;
    }
    
    // Process as HTML page
    monitoring->startProfiling("process_page");
    
    // Extract links from content
    std::vector<std::string> links = urlParser->extractLinks(content, url);
    
    // Add links to queue
    for (const auto& link : links) {
        // Check if domain is allowed
        std::string domain = urlParser->getDomain(link);
        std::vector<std::string> allowedDomains = config.getAllowedDomains();
        
        if (allowedDomains.empty() || 
            std::find(allowedDomains.begin(), allowedDomains.end(), domain) != allowedDomains.end()) {
            scheduleUrl(link, depth + 1);
        }
    }
    
    // Extract and process images
    std::vector<std::string> imageUrls = urlParser->extractImages(content, url);
    for (const auto& imageUrl : imageUrls) {
        scheduleUrl(imageUrl, depth + 1);
    }
    
    // Save page content to database and file system
    std::string filePath = fileIndexer->getPagePath(url);
    fileIndexer->savePage(url, content);
    database->addPage(url, "Page " + url, content, filePath);
    
    monitoring->stopProfiling("process_page");
    totalPages++;
    
    return true;
}

// Static callback for CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), realSize);
    return realSize;
}

bool WebCrawler::downloadPage(const std::string& url, std::string& content) {
    monitoring->startProfiling("download_page");
    
    content.clear();
    
    // Create CURL handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        monitoring->log(Monitoring::LogLevel::LOG_ERROR, "Failed to initialize CURL");
        return false;
    }
    
    // Set up CURL options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, config.getUserAgent().c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config.getTimeoutSeconds());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, config.getFollowRedirects() ? 1L : 0L);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Check for errors
    bool success = (res == CURLE_OK);
    if (success) {
        // Check HTTP status code
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        
        if (httpCode != 200) {
            monitoring->log(Monitoring::LogLevel::WARNING, 
                "HTTP error " + std::to_string(httpCode) + " for URL: " + url);
            success = false;
        }
        
        // Check content type
        char* contentType = nullptr;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);
        
        if (contentType && 
            !isImageUrl(url) && 
            std::string(contentType).find("text/html") == std::string::npos) {
            monitoring->log(Monitoring::LogLevel::WARNING, 
                "Skipping non-HTML content type: " + std::string(contentType ? contentType : "unknown") + " for URL: " + url);
            success = false;
        }
    } else {
        monitoring->log(Monitoring::LogLevel::LOG_ERROR, 
            "CURL error for URL: " + url + " - " + curl_easy_strerror(res));
    }
    
    // Clean up
    curl_easy_cleanup(curl);
    
    monitoring->stopProfiling("download_page");
    return success;
}

void WebCrawler::processImage(const std::string& url, const std::vector<uint8_t>& imageData) {
    monitoring->startProfiling("process_image");
    
    try {
        // Analyze image
        auto features = imageAnalyzer->analyzeImageData(imageData);
        
        // Skip NSFW images
        if (features.isNSFW) {
            monitoring->log(Monitoring::LogLevel::WARNING, "Skipping NSFW image: " + url);
            return;
        }
        
        // Save image data
        std::string extension = getImageExtension(url);
        
        // Use try/catch to handle potential errors in saveImage
        try {
            // Save the image using FileIndexer
            if (fileIndexer->saveImage(url, imageData, extension)) {
                // Save metadata to database
                std::string description = features.description.empty() ? "No description" : features.description;
                // Convert vectors to a single string for the database
                std::string labelsStr = vectorToString(features.labels);
                std::string objectsStr = vectorToString(features.objects);
                
                // Add image metadata to database
                try {
                    database->addImage(url, description, labelsStr, objectsStr);
                    monitoring->log(Monitoring::LogLevel::INFO, "Processed image: " + url);
                    imagesProcessed++;
                } catch (...) {
                    monitoring->log(Monitoring::LogLevel::LOG_ERROR, "Failed to add image metadata to database: " + url);
                    failedRequests++;
                }
            } else {
                monitoring->log(Monitoring::LogLevel::LOG_ERROR, "Failed to save image: " + url);
                failedRequests++;
            }
        } catch (...) {
            monitoring->log(Monitoring::LogLevel::LOG_ERROR, "Exception occurred while saving image: " + url);
            failedRequests++;
        }
    } catch (const std::exception& e) {
        monitoring->log(Monitoring::LogLevel::LOG_ERROR, "Failed to process image: " + url + " - " + e.what());
        failedRequests++;
    }
    
    monitoring->stopProfiling("process_image");
}

bool WebCrawler::isImageUrl(const std::string& url) {
    // Check file extension
    static const std::vector<std::string> imageExtensions = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".svg"
    };
    
    std::string lowerUrl = url;
    std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::tolower);
    
    for (const auto& ext : imageExtensions) {
        if (lowerUrl.size() >= ext.size() && 
            lowerUrl.substr(lowerUrl.size() - ext.size()) == ext) {
            return true;
        }
    }
    
    return false;
}

std::string WebCrawler::getImageExtension(const std::string& url) {
    // Extract file extension from URL
    std::regex extRegex("\\.([^./\\?]+)(?:[\\?#].*)?$");
    std::smatch match;
    
    if (std::regex_search(url, match, extRegex) && match.size() > 1) {
        return match[1].str();
    }
    
    // Default extension
    return "jpg";
}

// Helper function to convert vector to string
std::string WebCrawler::vectorToString(const std::vector<std::string>& vec) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i < vec.size() - 1) {
            result += ",";
        }
    }
    return result;
}
