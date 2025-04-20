#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "include/crawler.hpp"
#include "include/config.hpp"

int main(int argc, char* argv[]) {
    std::cout << "Wikipedia Web Crawler Test" << std::endl;
    std::cout << "------------------------" << std::endl;
    
    // Default Wikipedia URL
    std::string url = "https://en.wikipedia.org/wiki/Web_crawler";
    if (argc > 1) {
        url = argv[1];
    }
    
    // Create crawler with Wikipedia domain
    std::vector<std::string> allowedDomains = {"en.wikipedia.org"};
    size_t threads = 2;  // Keep it small for testing
    int maxDepth = 1;    // Only crawl direct links
    
    std::cout << "Starting crawl at: " << url << std::endl;
    std::cout << "Threads: " << threads << std::endl;
    std::cout << "Max depth: " << maxDepth << std::endl;
    std::cout << "Allowed domains: en.wikipedia.org" << std::endl;
    
    try {
        WebCrawler crawler(url, threads, maxDepth, allowedDomains);
        
        // Set user agent to avoid being blocked
        crawler.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) Test-Web-Crawler/1.0");
        
        // Start crawler
        std::cout << "\nStarting crawler..." << std::endl;
        
        // Create a separate thread for the crawler
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        // Monitor progress
        bool running = true;
        int dots = 0;
        while (running && dots < 30) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            size_t pages = crawler.getPagesCrawled();
            size_t queue = crawler.getQueueSize();
            size_t active = crawler.getActiveThreads();
            
            std::cout << "\rPages: " << pages
                      << " | Queue: " << queue
                      << " | Active threads: " << active
                      << " | " << std::string(dots % 4, '.') << "   ";
            std::cout.flush();
            
            dots++;
            
            // Auto-stop after 30 seconds for testing
            if (dots >= 30 || (pages > 0 && queue == 0 && active == 0)) {
                running = false;
            }
        }
        
        // Stop the crawler
        std::cout << "\n\nStopping crawler..." << std::endl;
        crawler.stop();
        
        // Wait for the crawler thread to finish
        if (crawlerThread.joinable()) {
            crawlerThread.join();
        }
        
        // Print final stats
        std::cout << "\nFinal statistics:" << std::endl;
        std::cout << "----------------" << std::endl;
        std::cout << "Pages crawled: " << crawler.getPagesCrawled() << std::endl;
        std::cout << "Failed requests: " << crawler.getFailedRequests() << std::endl;
        std::cout << "Total downloaded: " << crawler.getTotalBytesDownloaded() / 1024 << " KB" << std::endl;
        std::cout << "Average response time: " << crawler.getAverageResponseTime() << " ms" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
} 