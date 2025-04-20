#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "universal_crawler.cpp"

std::vector<std::string> splitString(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char* argv[]) {
    // Default values
    std::string startUrl = "https://example.com";
    int maxDepth = 2;
    std::vector<std::string> allowedDomains = {"example.com", "www.example.com"};
    
    // Parse command line arguments
    if (argc >= 2) {
        startUrl = argv[1];
    }
    
    if (argc >= 3) {
        try {
            maxDepth = std::stoi(argv[2]);
            if (maxDepth < 0) maxDepth = 0;
            if (maxDepth > 10) {
                std::cout << "Warning: High depth values may cause excessive crawling. Limiting to 10." << std::endl;
                maxDepth = 10;
            }
        } catch (const std::exception& e) {
            std::cerr << "Invalid depth value. Using default (2)." << std::endl;
        }
    }
    
    if (argc >= 4) {
        std::string domainsArg = argv[3];
        allowedDomains = splitString(domainsArg, ',');
    }
    
    std::cout << "Starting Universal Web Crawler" << std::endl;
    std::cout << "URL: " << startUrl << std::endl;
    std::cout << "Max Depth: " << maxDepth << std::endl;
    std::cout << "Allowed Domains: ";
    for (size_t i = 0; i < allowedDomains.size(); ++i) {
        std::cout << allowedDomains[i];
        if (i < allowedDomains.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl << std::endl;
    
    // Create and start crawler
    UniversalCrawler crawler;
    crawler.setAllowedDomains(allowedDomains);
    
    try {
        crawler.start(startUrl, maxDepth);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nCrawling complete!" << std::endl;
    std::cout << "Pages crawled: " << crawler.getPagesCrawled() << std::endl;
    std::cout << "Unique URLs: " << crawler.getUniqueUrls() << std::endl;
    std::cout << "Images saved: " << crawler.getImagesSaved() << std::endl;
    
    return 0;
} 