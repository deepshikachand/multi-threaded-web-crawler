#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <filesystem>
#include "crawler.hpp"
#include "config.hpp"
#include "../include/universal_crawler.hpp"

namespace fs = std::filesystem;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] [config_file]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --url <url>         Starting URL (overrides config file)" << std::endl;
    std::cout << "  --threads <num>     Number of threads to use (overrides config file)" << std::endl;
    std::cout << "  --depth <num>       Maximum crawl depth (overrides config file)" << std::endl;
    std::cout << "  --allowed-domains <domains>  Comma-separated list of allowed domains (overrides config file)" << std::endl;
    std::cout << "  --verbose           Enable verbose logging" << std::endl;
    std::cout << "  --stats-only        Only display database statistics without crawling" << std::endl;
    std::cout << "  --help              Display this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "If no config file is specified, default config.json will be used." << std::endl;
}

void displayStats(const UniversalCrawler& crawler) {
    std::cout << "Queue Size: " << crawler.getQueueSize() 
              << " | Pages Crawled: " << crawler.getPagesCrawled()
              << " | Images Saved: " << crawler.getImagesSaved()
              << " | Active Threads: " << crawler.getActiveThreads()
              << " | Unique URLs: " << crawler.getUniqueUrls() << std::endl;
}

// Parse a comma-separated string into a vector of strings
std::vector<std::string> parseCommaSeparatedList(const std::string& input) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

// Display crawl database statistics
void displayDatabaseStats() {
    std::cout << "\nDatabase Statistics:\n";
    std::cout << "------------------\n";
    
    // Check if data directory exists
    if (!fs::exists("data")) {
        std::cout << "No data directory found. No data has been saved.\n";
        return;
    }
    
    // Pages database
    std::string pagesFile = "data/crawled_pages.csv";
    if (fs::exists(pagesFile)) {
        std::ifstream file(pagesFile);
        if (file.is_open()) {
            int lineCount = 0;
            std::string line;
            
            // Skip header line
            std::getline(file, line);
            
            // Count domains
            std::unordered_map<std::string, int> domainCounts;
            std::unordered_map<int, int> depthCounts;
            
            while (std::getline(file, line)) {
                lineCount++;
                
                // Parse CSV line (simple approach)
                size_t firstComma = line.find(',');
                size_t secondComma = line.find(',', firstComma + 1);
                size_t thirdComma = line.find(',', secondComma + 1);
                
                if (firstComma != std::string::npos && secondComma != std::string::npos) {
                    std::string domain = line.substr(firstComma + 1, secondComma - firstComma - 1);
                    std::string depthStr = line.substr(secondComma + 1, thirdComma - secondComma - 1);
                    
                    domainCounts[domain]++;
                    depthCounts[std::stoi(depthStr)]++;
                }
            }
            
            file.close();
            
            std::cout << "Pages stored in database: " << lineCount << std::endl;
            std::cout << "Domains crawled: " << domainCounts.size() << std::endl;
            
            // Display top domains
            std::cout << "\nTop domains by page count:\n";
            std::vector<std::pair<std::string, int>> domainPairs(domainCounts.begin(), domainCounts.end());
            std::sort(domainPairs.begin(), domainPairs.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
            int count = 0;
            for (const auto& [domain, pages] : domainPairs) {
                std::cout << "  " << domain << ": " << pages << " pages\n";
                if (++count >= 5) break; // Show top 5
            }
            
            // Display pages by depth
            std::cout << "\nPages by crawl depth:\n";
            for (const auto& [depth, count] : depthCounts) {
                std::cout << "  Depth " << depth << ": " << count << " pages\n";
            }
        }
    } else {
        std::cout << "No pages database found.\n";
    }
    
    // Images database
    std::string imagesFile = "data/discovered_images.csv";
    if (fs::exists(imagesFile)) {
        std::ifstream file(imagesFile);
        if (file.is_open()) {
            int imageCount = 0;
            std::string line;
            
            // Skip header line
            std::getline(file, line);
            
            // Count image types
            std::unordered_map<std::string, int> imageTypeCounts;
            long long totalSizeKB = 0;
            
            while (std::getline(file, line)) {
                imageCount++;
                
                // Parse CSV line (simple approach)
                size_t firstComma = line.find(',');
                size_t secondComma = line.find(',', firstComma + 1);
                size_t thirdComma = line.find(',', secondComma + 1);
                
                if (firstComma != std::string::npos && secondComma != std::string::npos && thirdComma != std::string::npos) {
                    std::string imageType = line.substr(secondComma + 1, thirdComma - secondComma - 1);
                    std::string sizeStr = line.substr(thirdComma + 1, line.find(',', thirdComma + 1) - thirdComma - 1);
                    
                    imageTypeCounts[imageType]++;
                    totalSizeKB += std::stoll(sizeStr);
                }
            }
            
            file.close();
            
            std::cout << "\nImages found: " << imageCount << std::endl;
            if (imageCount > 0) {
                double avgSizeKB = static_cast<double>(totalSizeKB) / imageCount;
                std::cout << "Total image size: " << (totalSizeKB / 1024.0) << " MB\n";
                std::cout << "Average image size: " << avgSizeKB << " KB\n";
                
                // Display image types
                std::cout << "\nImage types:\n";
                for (const auto& [type, count] : imageTypeCounts) {
                    std::cout << "  " << type << ": " << count << " images (" 
                              << (count * 100.0 / imageCount) << "%)\n";
                }
            }
        }
    } else {
        std::cout << "\nNo images database found.\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Universal Web Crawler Demo\n";
    std::cout << "-------------------------\n\n";
    
    // Default values
    std::string seedUrl = "https://example.com";
    int maxThreads = 4;
    int maxDepth = 3;
    std::vector<std::string> allowedDomains = {"example.com", "sub.example.com"};
    bool verbose = false;
    bool statsOnly = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--url" && i + 1 < argc) {
            seedUrl = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            try {
                maxThreads = std::stoi(argv[++i]);
                if (maxThreads < 1) maxThreads = 1;
            } catch (const std::exception&) {
                std::cerr << "Invalid thread count. Using default: " << maxThreads << std::endl;
            }
        } else if (arg == "--depth" && i + 1 < argc) {
            try {
                maxDepth = std::stoi(argv[++i]);
                if (maxDepth < 0) maxDepth = 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid depth. Using default: " << maxDepth << std::endl;
            }
        } else if (arg == "--allowed-domains" && i + 1 < argc) {
            allowedDomains = parseCommaSeparatedList(argv[++i]);
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--stats-only") {
            statsOnly = true;
        }
    }
    
    // If stats-only mode, just display the stats and exit
    if (statsOnly) {
        displayDatabaseStats();
        return 0;
    }
    
    // Create crawler instance
    UniversalCrawler crawler;
    
    // Configure crawler
    crawler.setMaxThreads(maxThreads);
    crawler.setMaxDepth(maxDepth);
    
    // Set allowed domains
    if (!allowedDomains.empty()) {
        crawler.setAllowedDomains(allowedDomains);
    }
    
    // Display configuration
    std::cout << "Configuration:\n";
    std::cout << "- Starting URL: " << seedUrl << "\n";
    std::cout << "- Thread count: " << maxThreads << "\n";
    std::cout << "- Max depth: " << maxDepth << "\n";
    std::cout << "- Allowed domains: ";
    for (size_t i = 0; i < allowedDomains.size(); i++) {
        std::cout << allowedDomains[i];
        if (i < allowedDomains.size() - 1) std::cout << ", ";
    }
    std::cout << "\n\n";
    
    std::cout << "Starting crawler with seed URL: " << seedUrl << std::endl;
    std::cout << "Press Ctrl+C to stop...\n\n";
    
    // Start crawler
    crawler.start(seedUrl);
    
    // Monitor and display progress
    try {
        for (int i = 0; i < 30; i++) {
            displayStats(crawler);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    // Stop crawler
    std::cout << "\nStopping crawler...\n";
    crawler.stop();
    
    // Final statistics
    std::cout << "\nFinal Statistics:\n";
    std::cout << "----------------\n";
    displayStats(crawler);
    
    // Display database stats
    displayDatabaseStats();
    
    return 0;
} 