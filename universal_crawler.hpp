#pragma once

#include <string>
#include <vector>
#include <atomic>

// Universal web crawler class for multi-threaded website crawling
class UniversalCrawler {
public:
    // Default constructor
    UniversalCrawler();
    
    // Destructor
    ~UniversalCrawler();
    
    // Set allowed domains - limits crawling to these domains only
    void setAllowedDomains(const std::vector<std::string>& domains);
    
    // Set maximum number of threads for parallel crawling
    void setMaxThreads(int threads);
    
    // Add a URL to the crawl queue with its depth
    void addUrl(const std::string& url, int depth);
    
    // Start the crawler with the given start URL and maximum depth
    void start(const std::string& startUrl, int maxDepth = 2);
    
    // Stop the crawler (can be called from another thread)
    void stop();
    
    // Get the total number of pages crawled
    int getPagesCrawled() const;
    
    // Get the total number of unique URLs visited
    int getUniqueUrls() const;
    
    // Get the total number of images saved
    int getImagesSaved() const;
    
    // Get the current size of the URL queue
    int getQueueSize() const;
    
    // Get the number of currently active threads
    int getActiveThreads() const;
    
private:
    // Private implementation details are in the .cpp file
};

// URL parsing utility class
class URLParser {
public:
    // Extract domain from URL
    static std::string getDomain(const std::string& url);
    
    // Normalize relative URLs based on a base URL
    static std::string normalizeURL(const std::string& base, const std::string& url);
    
    // Check if a URL is valid
    static bool isValidUrl(const std::string& url);
}; 