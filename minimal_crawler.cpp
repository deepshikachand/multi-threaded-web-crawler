#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <set>
#include <regex>
#include <curl/curl.h>

// Simple minimal web crawler for testing

// Callback function for cURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

class MinimalCrawler {
public:
    MinimalCrawler(const std::string& startUrl, int maxDepth = 1) 
        : startUrl(startUrl), maxDepth(maxDepth), pagesCrawled(0), running(false) {
        
        // Initialize cURL
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    
    ~MinimalCrawler() {
        // Cleanup cURL
        curl_global_cleanup();
    }
    
    void start() {
        running = true;
        visitedUrls.clear();
        urlQueue = std::queue<std::pair<std::string, int>>();
        pagesCrawled = 0;
        
        // Add the start URL to the queue with depth 0
        urlQueue.push({startUrl, 0});
        
        std::cout << "Starting crawl at: " << startUrl << std::endl;
        
        while (running && !urlQueue.empty()) {
            auto [url, depth] = urlQueue.front();
            urlQueue.pop();
            
            // Skip if already visited or depth exceeds maximum
            if (visitedUrls.find(url) != visitedUrls.end() || depth > maxDepth) {
                continue;
            }
            
            // Mark as visited
            visitedUrls.insert(url);
            
            // Crawl the URL
            std::string content = fetchUrl(url);
            if (!content.empty()) {
                pagesCrawled++;
                
                // Extract title
                std::string title = extractTitle(content);
                std::cout << "Crawled: " << title << " (" << url << ")" << std::endl;
                
                // Extract links if we haven't reached max depth
                if (depth < maxDepth) {
                    auto links = extractLinks(content, url);
                    std::cout << "  Found " << links.size() << " links." << std::endl;
                    
                    // Add links to queue
                    for (const auto& link : links) {
                        urlQueue.push({link, depth + 1});
                    }
                }
            }
            
            // Sleep briefly to avoid overloading the server
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        std::cout << "Crawling completed!" << std::endl;
        std::cout << "Pages crawled: " << pagesCrawled << std::endl;
    }
    
    void stop() {
        running = false;
    }
    
    int getPagesCrawled() const {
        return pagesCrawled;
    }
    
private:
    std::string fetchUrl(const std::string& url) {
        CURL* curl = curl_easy_init();
        std::string content;
        
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Test-Crawler/1.0");
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10); // 10 seconds timeout
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
            
            CURLcode res = curl_easy_perform(curl);
            
            if (res != CURLE_OK) {
                std::cerr << "Failed to fetch " << url << ": " 
                          << curl_easy_strerror(res) << std::endl;
                content.clear();
            } else {
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                
                if (http_code != 200) {
                    std::cerr << "HTTP code " << http_code << " for " << url << std::endl;
                    content.clear();
                }
            }
            
            curl_easy_cleanup(curl);
        }
        
        return content;
    }
    
    std::string extractTitle(const std::string& content) {
        std::regex titleRegex("<title>([^<]+)</title>");
        std::smatch match;
        
        if (std::regex_search(content, match, titleRegex) && match.size() > 1) {
            return match[1].str();
        }
        
        return "No title";
    }
    
    std::vector<std::string> extractLinks(const std::string& content, const std::string& baseUrl) {
        std::vector<std::string> links;
        std::set<std::string> uniqueLinks;
        
        // Only accept Wikipedia links
        std::regex linkRegex("href=\"(/wiki/[^\"#:]+)\"");
        std::smatch match;
        std::string::const_iterator searchStart(content.cbegin());
        
        while (std::regex_search(searchStart, content.cend(), match, linkRegex)) {
            std::string path = match[1].str();
            
            // Skip special pages
            if (path.find("/wiki/Special:") == std::string::npos &&
                path.find("/wiki/Help:") == std::string::npos &&
                path.find("/wiki/Talk:") == std::string::npos &&
                path.find("/wiki/Wikipedia:") == std::string::npos &&
                path.find("/wiki/Template:") == std::string::npos &&
                path.find("/wiki/File:") == std::string::npos) {
                
                // Create absolute URL
                std::string fullUrl = "https://en.wikipedia.org" + path;
                
                // Add to links if not already seen
                if (uniqueLinks.find(fullUrl) == uniqueLinks.end()) {
                    uniqueLinks.insert(fullUrl);
                    links.push_back(fullUrl);
                }
            }
            
            searchStart = match.suffix().first;
        }
        
        return links;
    }
    
    std::string startUrl;
    int maxDepth;
    int pagesCrawled;
    bool running;
    
    std::queue<std::pair<std::string, int>> urlQueue;
    std::set<std::string> visitedUrls;
};

int main(int argc, char* argv[]) {
    std::cout << "Minimal Web Crawler Test" << std::endl;
    std::cout << "----------------------" << std::endl;
    
    // Default Wikipedia URL
    std::string url = "https://en.wikipedia.org/wiki/Web_crawler";
    if (argc > 1) {
        url = argv[1];
    }
    
    // Default max depth
    int maxDepth = 1;
    if (argc > 2) {
        maxDepth = std::stoi(argv[2]);
    }
    
    std::cout << "URL: " << url << std::endl;
    std::cout << "Max depth: " << maxDepth << std::endl;
    std::cout << std::endl;
    
    // Create and run the crawler
    try {
        MinimalCrawler crawler(url, maxDepth);
        crawler.start();
        
        std::cout << "\nCrawling completed with " << crawler.getPagesCrawled() 
                  << " pages crawled!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 