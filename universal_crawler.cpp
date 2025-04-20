#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <regex>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <curl/curl.h>
#include <random>
#include "include/universal_crawler.hpp"

namespace fs = std::filesystem;

class URLParser {
public:
    static std::string getDomain(const std::string& url) {
        std::regex domainRegex("https?://([^/]+)");
        std::smatch match;
        if (std::regex_search(url, match, domainRegex) && match.size() > 1) {
            return match[1].str();
        }
        return "";
    }

    static std::string normalizeUrl(const std::string& url) {
        // Basic URL normalization - could be extended
        std::string result = url;
        
        // Remove fragments
        size_t fragmentPos = result.find('#');
        if (fragmentPos != std::string::npos) {
            result = result.substr(0, fragmentPos);
        }
        
        // Remove trailing slash if present
        if (!result.empty() && result.back() == '/') {
            result.pop_back();
        }
        
        return result;
    }
    
    static bool isValidUrl(const std::string& url) {
        std::regex urlRegex("^https?://[^\\s/$.?#].[^\\s]*$");
        return std::regex_match(url, urlRegex);
    }

    static int getDepthFromUrl(const std::string& url) {
        // Count the number of directory levels in the URL path
        std::string domain = getDomain(url);
        if (domain.empty()) return 0;
        
        size_t domainPos = url.find(domain);
        if (domainPos == std::string::npos) return 0;
        
        std::string path = url.substr(domainPos + domain.length());
        
        int depth = 0;
        for (char c : path) {
            if (c == '/') depth++;
        }
        
        return depth;
    }
};

class UniversalCrawler {
private:
    std::queue<std::pair<std::string, int>> urlQueue;
    std::set<std::string> visitedUrls;
    std::vector<std::string> allowedDomains;
    std::set<std::string> excludedExtensions = {".pdf", ".doc", ".docx", ".xls", ".xlsx", ".zip", ".rar", ".exe", ".mp3", ".mp4", ".avi"};
    
    std::atomic<int> pagesCrawled{0};
    std::atomic<int> imagesSaved{0};
    std::atomic<int> activeThreads{0};
    
    std::mutex queueMutex;
    std::mutex visitedMutex;
    std::mutex logMutex;
    std::condition_variable queueCondition;
    
    bool shouldStop = false;
    int maxThreads = 5;
    
    // Curl handles
    CURLM* multiHandle = nullptr;
    std::map<CURL*, std::string> handleToUrl;
    
    // Helper function to check if a URL domain is allowed
    bool isDomainAllowed(const std::string& url) {
        std::string domain = URLParser::getDomain(url);
        
        if (allowedDomains.empty()) return true;
        
        for (const auto& allowedDomain : allowedDomains) {
            if (domain == allowedDomain || 
                (domain.length() > allowedDomain.length() && 
                 domain.substr(domain.length() - allowedDomain.length() - 1) == "." + allowedDomain)) {
                return true;
            }
        }
        return false;
    }
    
    // Helper function to check if a URL has an excluded extension
    bool hasExcludedExtension(const std::string& url) {
        for (const auto& ext : excludedExtensions) {
            if (url.length() > ext.length() && 
                url.substr(url.length() - ext.length()) == ext) {
                return true;
            }
        }
        return false;
    }
    
    // CURL write callback
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    // Extract links from HTML content
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl) {
        std::vector<std::string> links;
        std::regex linkRegex(R"(<a\s+[^>]*href=["']([^"']+)["'][^>]*>)");
        
        auto linksBegin = std::sregex_iterator(html.begin(), html.end(), linkRegex);
        auto linksEnd = std::sregex_iterator();
        
        for (std::sregex_iterator i = linksBegin; i != linksEnd; ++i) {
            std::string link = (*i)[1].str();
            std::string normalizedLink = URLParser::normalizeUrl(link);
            
            if (!normalizedLink.empty() && URLParser::isValidUrl(normalizedLink) && 
                !hasExcludedExtension(normalizedLink)) {
                links.push_back(normalizedLink);
            }
        }
        
        return links;
    }
    
    // Extract images from HTML content
    std::vector<std::pair<std::string, std::string>> extractImages(const std::string& html, const std::string& baseUrl) {
        std::vector<std::pair<std::string, std::string>> images;
        std::regex imageRegex(R"(<img\s+[^>]*src=["']([^"']+)["'][^>]*(?:alt=["']([^"']*)["'])?[^>]*>)");
        
        auto imagesBegin = std::sregex_iterator(html.begin(), html.end(), imageRegex);
        auto imagesEnd = std::sregex_iterator();
        
        for (std::sregex_iterator i = imagesBegin; i != imagesEnd; ++i) {
            std::smatch match = *i;
            std::string imageUrl = match[1].str();
            std::string alt = match.size() > 2 ? match[2].str() : "";
            
            std::string normalizedImageUrl = URLParser::normalizeUrl(imageUrl);
            
            if (!normalizedImageUrl.empty() && URLParser::isValidUrl(normalizedImageUrl)) {
                images.push_back({normalizedImageUrl, alt});
            }
        }
        
        return images;
    }
    
    // Download and process a single page
    void processPage(const std::string& url, int depth) {
        log("Processing: " + url + " (depth: " + std::to_string(depth) + ")");
        
        // Initialize CURL
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string pageContent;
            
            // Set CURL options
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pageContent);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 Universal-Web-Crawler/1.0");
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            
            // Perform the request
            CURLcode res = curl_easy_perform(curl);
            
            if (res == CURLE_OK) {
                pagesCrawled++;
                
                // Extract page title
                std::string title = "";
                std::regex titleRegex(R"(<title>([^<]+)</title>)");
                std::smatch titleMatch;
                if (std::regex_search(pageContent, titleMatch, titleRegex) && titleMatch.size() > 1) {
                    title = titleMatch[1].str();
                }
                
                log("Title: " + title);
                
                // Save page content
                saveContent(url, title, pageContent);
                
                // Extract and process links if not at max depth
                if (depth < getMaxDepth()) {
                    std::vector<std::string> links = extractLinks(pageContent, url);
                    log("Found " + std::to_string(links.size()) + " links");
                    
                    for (const auto& link : links) {
                        if (isDomainAllowed(link)) {
                            addUrl(link, depth + 1);
                        }
                    }
                }
                
                // Extract and process images
                std::vector<std::pair<std::string, std::string>> images = extractImages(pageContent, url);
                log("Found " + std::to_string(images.size()) + " images");
                
                for (const auto& image : images) {
                    downloadImage(image.first, image.second);
                }
            } else {
                log("Failed to download: " + url + " - " + curl_easy_strerror(res), true);
            }
            
            // Clean up
            curl_easy_cleanup(curl);
        }
    }
    
    // Download and save an image
    void downloadImage(const std::string& imageUrl, const std::string& alt) {
        // Create a unique filename based on the URL
        std::string filename = "data/images/" + std::to_string(std::hash<std::string>{}(imageUrl)) + getImageExtension(imageUrl);
        
        // Check if the image already exists
        if (fs::exists(filename)) {
            return;
        }
        
        // Initialize CURL
        CURL* curl = curl_easy_init();
        if (curl) {
            // Open file for writing
            FILE* fp = fopen(filename.c_str(), "wb");
            if (fp) {
                // Set CURL options
                curl_easy_setopt(curl, CURLOPT_URL, imageUrl.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 Universal-Web-Crawler/1.0");
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
                
                // Perform the request
                CURLcode res = curl_easy_perform(curl);
                
                // Close the file
                fclose(fp);
                
                if (res == CURLE_OK) {
                    imagesSaved++;
                    log("Saved image: " + imageUrl);
                    
                    // Save image metadata
                    saveImageMetadata(imageUrl, filename, alt);
                } else {
                    log("Failed to download image: " + imageUrl + " - " + curl_easy_strerror(res), true);
                    fs::remove(filename);
                }
            }
            
            // Clean up
            curl_easy_cleanup(curl);
        }
    }
    
    // Get image file extension from URL
    std::string getImageExtension(const std::string& url) {
        std::regex extensionRegex(R"(\.(jpg|jpeg|png|gif|bmp|webp)($|\?.*$))");
        std::smatch match;
        if (std::regex_search(url, match, extensionRegex) && match.size() > 1) {
            return "." + match[1].str();
        }
        return ".jpg";  // Default extension
    }
    
    // Save page content to file
    void saveContent(const std::string& url, const std::string& title, const std::string& content) {
        std::string filename = "data/content/" + std::to_string(std::hash<std::string>{}(url)) + ".html";
        
        std::ofstream file(filename);
        if (file.is_open()) {
            file << content;
            file.close();
            
            // Save metadata
            saveContentMetadata(url, filename, title);
        }
    }
    
    // Save content metadata to file
    void saveContentMetadata(const std::string& url, const std::string& filename, const std::string& title) {
        std::string metadataFile = "data/content/metadata.csv";
        bool fileExists = fs::exists(metadataFile);
        
        std::ofstream file(metadataFile, std::ios::app);
        if (file.is_open()) {
            if (!fileExists) {
                file << "URL,Filename,Title,Timestamp\n";
            }
            
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::system_clock::to_time_t(now);
            
            file << url << "," 
                 << filename << "," 
                 << "\"" << title << "\"," 
                 << timestamp << "\n";
            
            file.close();
        }
    }
    
    // Save image metadata to file
    void saveImageMetadata(const std::string& url, const std::string& filename, const std::string& alt) {
        std::string metadataFile = "data/images/metadata.csv";
        bool fileExists = fs::exists(metadataFile);
        
        std::ofstream file(metadataFile, std::ios::app);
        if (file.is_open()) {
            if (!fileExists) {
                file << "URL,Filename,Alt,Timestamp\n";
            }
            
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::system_clock::to_time_t(now);
            
            file << url << "," 
                 << filename << "," 
                 << "\"" << alt << "\"," 
                 << timestamp << "\n";
            
            file.close();
        }
    }
    
    // Log a message with timestamp
    void log(const std::string& message, bool isError = false) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        
        std::string timeStr = std::ctime(&timestamp);
        timeStr.pop_back(); // Remove trailing newline
        
        std::ofstream logFile("logs/crawler.log", std::ios::app);
        logFile << "[" << timeStr << "] " << (isError ? "ERROR: " : "") << message << std::endl;
        logFile.close();
        
        if (isError) {
            std::cerr << message << std::endl;
        } else {
            std::cout << message << std::endl;
        }
    }
    
    // Worker thread function
    void workerThread() {
        while (!shouldStop) {
            std::string url;
            int depth = 0;
            
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(lock, [this]{ 
                    return !urlQueue.empty() || shouldStop; 
                });
                
                if (shouldStop) break;
                
                if (!urlQueue.empty()) {
                    auto [nextUrl, nextDepth] = urlQueue.front();
                    urlQueue.pop();
                    url = nextUrl;
                    depth = nextDepth;
                    activeThreads++;
                }
            }
            
            if (!url.empty()) {
                try {
                    processPage(url, depth);
                } catch (const std::exception& e) {
                    log("Exception processing " + url + ": " + e.what(), true);
                }
                
                activeThreads--;
                
                // Notify if queue is empty and all threads are done
                if (activeThreads == 0 && urlQueue.empty()) {
                    shouldStop = true;
                    queueCondition.notify_all();
                }
            }
        }
    }
    
    // Get max depth for crawling
    int getMaxDepth() const {
        return 3;  // Default max depth
    }

public:
    UniversalCrawler() {
        // Create necessary directories
        fs::create_directories("data/content");
        fs::create_directories("data/images");
        fs::create_directories("logs");
        
        // Initialize CURL
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    ~UniversalCrawler() {
        // Clean up CURL
        curl_global_cleanup();
    }
    
    // Set allowed domains
    void setAllowedDomains(const std::vector<std::string>& domains) {
        std::lock_guard<std::mutex> lock(visitedMutex);
        allowedDomains = domains;
    }
    
    // Set max threads
    void setMaxThreads(int threads) {
        maxThreads = threads;
    }
    
    // Add a URL to the crawl queue
    void addUrl(const std::string& url, int depth) {
        std::lock_guard<std::mutex> visitedLock(visitedMutex);
        
        // Check if URL has already been visited
        if (visitedUrls.find(url) != visitedUrls.end()) {
            return;
        }
        
        visitedUrls.insert(url);
        
        std::lock_guard<std::mutex> queueLock(queueMutex);
        urlQueue.push({url, depth});
        queueCondition.notify_one();
    }
    
    // Start the crawler with the given URL
    void start(const std::string& startUrl, int maxDepth = 2) {
        if (!URLParser::isValidUrl(startUrl)) {
            throw std::runtime_error("Invalid start URL: " + startUrl);
        }
        
        log("Starting crawler with URL: " + startUrl + ", Max Depth: " + std::to_string(maxDepth));
        
        // Add the start URL to the queue
        addUrl(startUrl, 0);
        
        // Create worker threads
        std::vector<std::thread> threads;
        for (int i = 0; i < maxThreads; i++) {
            threads.push_back(std::thread(&UniversalCrawler::workerThread, this));
        }
        
        // Wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();
        }
        
        log("Crawler finished. Processed " + std::to_string(pagesCrawled) + " pages and saved " + std::to_string(imagesSaved) + " images.");
    }
    
    // Stop the crawler
    void stop() {
        log("Stopping crawler...");
        shouldStop = true;
        queueCondition.notify_all();
    }
    
    // Get statistics
    int getPagesCrawled() const { return pagesCrawled; }
    int getUniqueUrls() const { return visitedUrls.size(); }
    int getImagesSaved() const { return imagesSaved; }
    int getQueueSize() const { 
        std::lock_guard<std::mutex> lock(queueMutex);
        return urlQueue.size(); 
    }
    int getActiveThreads() const { return activeThreads; }
};

int main(int argc, char* argv[]) {
    std::cout << "Universal Web Crawler" << std::endl;
    std::cout << "====================" << std::endl;
    
    // Default values
    std::string url = "https://example.com";
    int maxDepth = 2;
    std::vector<std::string> allowedDomains = {"example.com", "www.example.com"};
    
    // Parse command line arguments
    if (argc > 1) {
        url = argv[1];
    }
    
    if (argc > 2) {
        maxDepth = std::stoi(argv[2]);
    }
    
    if (argc > 3) {
        allowedDomains.clear();
        std::string domains = argv[3];
        size_t pos = 0;
        std::string token;
        while ((pos = domains.find(',')) != std::string::npos) {
            token = domains.substr(0, pos);
            allowedDomains.push_back(token);
            domains.erase(0, pos + 1);
        }
        allowedDomains.push_back(domains);
    }
    
    std::cout << "URL: " << url << std::endl;
    std::cout << "Max depth: " << maxDepth << std::endl;
    std::cout << "Allowed domains:";
    for (const auto& domain : allowedDomains) {
        std::cout << " " << domain;
    }
    std::cout << std::endl << std::endl;
    
    try {
        // Create and run the crawler
        UniversalCrawler crawler;
        crawler.setAllowedDomains(allowedDomains);
        crawler.setMaxThreads(5); // 5 threads
        crawler.start(url, maxDepth);
        
        std::cout << "\nCrawling completed with " << crawler.getPagesCrawled() 
                  << " pages crawled!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 