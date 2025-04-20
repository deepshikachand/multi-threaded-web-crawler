#include "include/universal_crawler.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <signal.h>

// Global flag for handling Ctrl+C
volatile sig_atomic_t running = 1;

// Signal handler for Ctrl+C
void signalHandler(int signum) {
    running = 0;
}

int main() {
    // Set up signal handler
    signal(SIGINT, signalHandler);

    // Create and configure the crawler
    UniversalCrawler crawler;
    
    // Set the number of threads
    crawler.setMaxThreads(4);
    
    // Set max depth
    crawler.setMaxDepth(2);
    
    // Set allowed domains - we'll crawl developer.mozilla.org
    std::vector<std::string> allowedDomains = {"developer.mozilla.org"};
    crawler.setAllowedDomains(allowedDomains);
    
    // Start the crawler with a seed URL
    std::string seedUrl = "https://developer.mozilla.org/en-US/docs/Web";
    std::cout << "Starting crawler with seed URL: " << seedUrl << "\n";
    std::cout << "Press Ctrl+C to stop crawling\n\n";
    
    // Start crawler in a separate thread
    std::thread crawlerThread([&crawler, seedUrl]() {
        crawler.start(seedUrl);
    });
    
    // Monitor crawler status until stopped
    while (running) {
        std::cout << "\rStatus: "
                  << "Queue: " << crawler.getQueueSize() 
                  << " | Pages: " << crawler.getPagesCrawled() 
                  << " | Images: " << crawler.getImagesSaved()
                  << " | Threads: " << crawler.getActiveThreads()
                  << " | URLs: " << crawler.getUniqueUrls() << "    ";
        std::cout.flush();
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Also stop if crawler is done (empty queue and no active threads)
        if (crawler.getQueueSize() == 0 && crawler.getActiveThreads() == 0) {
            break;
        }
    }
    
    // Stop the crawler
    std::cout << "\n\nStopping crawler...\n";
    crawler.stop();
    
    // Wait for crawler thread to finish
    if (crawlerThread.joinable()) {
        crawlerThread.join();
    }
    
    // Display final statistics
    std::cout << "\nCrawl completed!\n";
    std::cout << "----------------\n";
    std::cout << "Total pages crawled: " << crawler.getPagesCrawled() << "\n";
    std::cout << "Total images saved: " << crawler.getImagesSaved() << "\n";
    std::cout << "Total unique URLs: " << crawler.getUniqueUrls() << "\n";
    
    return 0;
} 