#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include "universal_crawler.hpp"

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string startUrl = "https://example.com";
    int maxDepth = 2;
    int maxThreads = 4;
    
    // Allow override from command line
    if (argc > 1) startUrl = argv[1];
    if (argc > 2) maxDepth = std::stoi(argv[2]);
    if (argc > 3) maxThreads = std::stoi(argv[3]);
    
    std::cout << "Starting crawler with:" << std::endl;
    std::cout << "  URL: " << startUrl << std::endl;
    std::cout << "  Max depth: " << maxDepth << std::endl;
    std::cout << "  Threads: " << maxThreads << std::endl;
    
    // Configure and start the crawler
    UniversalCrawler crawler;
    
    // Set allowed domains to only crawl within the same domain
    std::vector<std::string> allowedDomains = {URLParser::getDomain(startUrl)};
    crawler.setAllowedDomains(allowedDomains);
    
    // Set number of threads
    crawler.setMaxThreads(maxThreads);
    
    // Start the crawler in a separate thread
    std::thread crawlerThread([&crawler, &startUrl, maxDepth]() {
        crawler.start(startUrl, maxDepth);
    });
    
    // Monitor and display progress
    bool running = true;
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Print status information
        std::cout << "\r"; // Clear line
        std::cout << "Queue: " << crawler.getQueueSize()
                  << " | Crawled: " << crawler.getPagesCrawled()
                  << " | Images: " << crawler.getImagesSaved()
                  << " | Active threads: " << crawler.getActiveThreads()
                  << "    ";
        std::cout.flush();
        
        // Check if crawler has finished
        if (crawler.getQueueSize() == 0 && crawler.getActiveThreads() == 0) {
            running = false;
        }
        
        // Allow user to stop crawler with Ctrl+C (handled elsewhere)
    }
    
    std::cout << std::endl << "Crawler finished!" << std::endl;
    std::cout << "Total pages crawled: " << crawler.getPagesCrawled() << std::endl;
    std::cout << "Total unique URLs: " << crawler.getUniqueUrls() << std::endl;
    std::cout << "Total images saved: " << crawler.getImagesSaved() << std::endl;
    
    // Wait for crawler thread to finish
    if (crawlerThread.joinable()) {
        crawlerThread.join();
    }
    
    return 0;
} 