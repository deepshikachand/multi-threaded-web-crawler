#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include "include/crawler.hpp"
#include "include/config.hpp"

// Global flag for handling Ctrl+C
std::atomic<bool> g_running{true};

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    std::cout << "\nShutting down crawler..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    // Set up signal handler for graceful termination
    std::signal(SIGINT, signalHandler);
    
    std::cout << "=== Wikipedia Web Crawler Test ===" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Load configuration
    std::string configFile = "config.json";
    if (argc > 1) {
        configFile = argv[1];
    }
    
    Config config;
    if (!config.loadConfig(configFile)) {
        std::cerr << "Failed to load configuration from " << configFile << std::endl;
        return 1;
    }
    
    std::cout << "Configuration loaded from " << configFile << std::endl;
    
    // Get crawler settings
    std::string url = config.getStartUrl();
    int threads = config.getThreadCount();
    int depth = config.getMaxDepth();
    std::vector<std::string> allowedDomains = config.getAllowedDomains();
    
    // Print crawl settings
    std::cout << "\nCrawl settings:" << std::endl;
    std::cout << "URL: " << url << std::endl;
    std::cout << "Threads: " << threads << std::endl;
    std::cout << "Depth: " << depth << std::endl;
    std::cout << "Allowed domains:";
    for (const auto& domain : allowedDomains) {
        std::cout << " " << domain;
    }
    std::cout << std::endl;
    
    // Create crawler
    try {
        WebCrawler crawler(url, threads, depth, allowedDomains);
        
        // Configure crawler
        crawler.setUserAgent(config.getUserAgent());
        crawler.setTimeoutSeconds(config.getTimeoutSeconds());
        crawler.setFollowRedirects(config.getFollowRedirects());
        crawler.setRetryCount(config.getRetryCount());
        
        // Start crawler in a separate thread
        std::cout << "\nStarting crawler...\n" << std::endl;
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        // Monitor crawler progress and allow for graceful shutdown
        int updateInterval = config.getStatusUpdateInterval();
        int totalSeconds = 0;
        
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            totalSeconds++;
            
            // Update statistics at specified interval
            if (totalSeconds % updateInterval == 0) {
                size_t pagesCrawled = crawler.getPagesCrawled();
                size_t queueSize = crawler.getQueueSize();
                size_t activeThreads = crawler.getActiveThreads();
                
                std::cout << "\r"; // Clear line
                std::cout << "Time: " << totalSeconds << "s | "
                          << "Pages: " << pagesCrawled << " | "
                          << "Queue: " << queueSize << " | "
                          << "Threads: " << activeThreads << " | "
                          << "Failed: " << crawler.getFailedRequests();
                std::cout.flush();
                
                // Auto-stop if crawler is done or after a time limit
                if ((pagesCrawled > 0 && queueSize == 0 && activeThreads == 0) || 
                    totalSeconds > 300) { // 5 minute time limit
                    g_running = false;
                }
            }
        }
        
        // Stop the crawler
        std::cout << "\n\nStopping crawler..." << std::endl;
        crawler.stop();
        
        // Wait for the crawler thread to finish
        if (crawlerThread.joinable()) {
            crawlerThread.join();
        }
        
        // Print final statistics
        std::cout << "\nFinal Statistics:" << std::endl;
        std::cout << "----------------" << std::endl;
        std::cout << "Pages crawled: " << crawler.getPagesCrawled() << std::endl;
        std::cout << "Queue size: " << crawler.getQueueSize() << std::endl;
        std::cout << "Active threads: " << crawler.getActiveThreads() << std::endl;
        std::cout << "Failed requests: " << crawler.getFailedRequests() << std::endl;
        std::cout << "Average response time: " << crawler.getAverageResponseTime() << " ms" << std::endl;
        std::cout << "Total data downloaded: " << crawler.getTotalBytesDownloaded() / 1024 << " KB" << std::endl;
        
        std::cout << "\nCrawl completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 