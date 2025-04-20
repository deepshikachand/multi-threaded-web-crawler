#pragma once

#include <string>
#include <vector>
#include <queue>
#include <unordered_set>
#include <mutex>
#include <thread>
#include <condition_variable>

// Simple URL parser class
class SimpleURLParser {
public:
    SimpleURLParser() = default;
    
    // Static methods
    static bool isValidUrl(const std::string& url);
    static std::string extractDomain(const std::string& url);
    
    // Instance methods
    bool parse(const std::string& url);
    std::string getDomain() const;
    std::string getPath() const;
    std::string getScheme() const;
    
private:
    std::string scheme;
    std::string domain;
    std::string path;
};

/**
 * @class UniversalCrawler
 * @brief A simplified web crawler implementation for educational purposes
 * 
 * This class provides a basic web crawler that can be used to demonstrate
 * multi-threaded web crawling concepts. It simulates the crawling process
 * without actually making real HTTP requests.
 */
class UniversalCrawler {
public:
    UniversalCrawler();
    ~UniversalCrawler();
    
    // Configuration methods
    void setMaxThreads(int threads);
    void setMaxDepth(int depth);
    void setAllowedDomains(const std::vector<std::string>& domains);
    
    // Control methods
    void start(const std::string& seedUrl, int depth = 0);
    void stop();
    
    // Status methods
    int getQueueSize() const;
    int getPagesCrawled() const;
    int getImagesSaved() const;
    int getActiveThreads() const;
    int getUniqueUrls() const;
    bool isRunning() const;
    
private:
    // Directory management
    void ensureDirectoriesExist();
    
    // Threading
    void crawlThread();
    void processUrl(const std::string& url, int depth);
    
    // URL generation helpers
    void generateWikipediaUrls(const std::string& domain, std::vector<std::string>& urls);
    void generateGitHubUrls(const std::string& domain, std::vector<std::string>& urls);
    void generateStackOverflowUrls(const std::string& domain, std::vector<std::string>& urls);
    void generateGenericUrls(const std::string& domain, std::vector<std::string>& urls);
    void addSubdomains(const std::string& domain, std::vector<std::string>& urls);
    
    // Data storage and simulation
    void savePageToDatabase(const std::string& url, int depth);
    void savePageContent(const std::string& url, int depth);
    void saveImageMetadata(const std::string& domain, int imageIndex);
    void saveImageToFileSystem(const std::string& domain, const std::string& imageName, 
                              const std::string& imageType, int imageIndex);
    int simulateImageDiscovery(const std::string& domain);
    
    // Thread management
    std::vector<std::thread> threads;
    mutable std::mutex mutex;
    std::condition_variable queueCondition;
    
    // URL queue and tracking
    std::queue<std::pair<std::string, int>> urlQueue; // URL and depth
    std::unordered_set<std::string> visitedUrls;
    
    // Configuration
    int maxThreads;
    int maxDepth;
    std::vector<std::string> allowedDomains;
    
    // State
    bool running;
    int pagesCrawled;
    int imagesSaved;
    int activeThreads;
}; 