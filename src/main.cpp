#include "crawler.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

std::atomic<bool> g_running(true);

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Stopping crawler..." << std::endl;
    g_running = false;
}

void printStats(const WebCrawler& crawler) {
    std::cout << "\rPages Crawled: " << crawler.getPagesCrawled()
              << " | Queue Size: " << crawler.getQueueSize()
              << " | Active Threads: " << crawler.getActiveThreads()
              << " | Failed Requests: " << crawler.getFailedRequests()
              << " | Avg Response Time: " << std::fixed << std::setprecision(2)
              << crawler.getAverageResponseTime() << "s"
              << " | Total Downloaded: " << (crawler.getTotalBytesDownloaded() / 1024 / 1024)
              << "MB" << std::flush;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <start_url> [num_threads] [max_depth]" << std::endl;
        return 1;
    }

    // Parse command line arguments
    std::string startUrl = argv[1];
    size_t numThreads = (argc > 2) ? std::stoul(argv[2]) : 4;
    int maxDepth = (argc > 3) ? std::stoi(argv[3]) : 3;

    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        // Initialize crawler
        std::vector<std::string> allowedDomains = {
            "example.com",
            "test.com"
        };

        WebCrawler crawler(startUrl, numThreads, maxDepth, allowedDomains);

        // Start crawler in a separate thread
        std::thread crawlerThread(&WebCrawler::start, &crawler);

        // Monitor crawler progress
        while (g_running) {
            printStats(crawler);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Stop crawler and wait for completion
        crawler.stop();
        if (crawlerThread.joinable()) {
            crawlerThread.join();
        }

        std::cout << "\nCrawler stopped successfully." << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
} 