#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <mutex>
#include <thread>
#include <curl/curl.h>
#include <sqlite3.h>
#include <regex>
#include <chrono>
#include <unordered_set>
#include <atomic>

// Thread-safe queue for URLs
class URLQueue {
private:
    std::queue<std::string> urls;
    std::mutex mutex;
    std::set<std::string> visited;

public:
    bool push(const std::string& url) {
        std::lock_guard<std::mutex> lock(mutex);
        // Only add URLs we haven't seen before
        if (visited.find(url) == visited.end()) {
            urls.push(url);
            visited.insert(url);
            return true;
        }
        return false;
    }

    bool pop(std::string& url) {
        std::lock_guard<std::mutex> lock(mutex);
        if (urls.empty()) {
            return false;
        }
        url = urls.front();
        urls.pop();
        return true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex);
        return urls.size();
    }
};

// Callback for curl to write data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Extract links from HTML
std::vector<std::string> extractLinks(const std::string& html, const std::string& base_url) {
    std::vector<std::string> links;
    std::regex linkRegex("<a[^>]+href=\"([^\"]+)\"");
    
    auto links_begin = std::sregex_iterator(html.begin(), html.end(), linkRegex);
    auto links_end = std::sregex_iterator();

    for (std::sregex_iterator i = links_begin; i != links_end; ++i) {
        std::smatch match = *i;
        std::string link = match[1].str();
        
        // Handle relative URLs
        if (link.substr(0, 4) != "http") {
            if (link[0] == '/') {
                // Extract domain from base_url
                size_t pos = base_url.find("://");
                if (pos != std::string::npos) {
                    pos += 3;
                    size_t endPos = base_url.find("/", pos);
                    if (endPos != std::string::npos) {
                        std::string domain = base_url.substr(0, endPos);
                        link = domain + link;
                    } else {
                        link = base_url + link;
                    }
                }
            } else {
                // Non-absolute path
                if (base_url.back() != '/') {
                    link = base_url + "/" + link;
                } else {
                    link = base_url + link;
                }
            }
        }
        
        links.push_back(link);
    }
    
    return links;
}

// Initialize database
sqlite3* initDatabase() {
    sqlite3* db;
    int rc = sqlite3_open("crawler.db", &db);
    
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return nullptr;
    }
    
    // Create table for pages
    const char* createTableSQL = 
        "CREATE TABLE IF NOT EXISTS pages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "url TEXT UNIQUE,"
        "title TEXT,"
        "content TEXT,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";
    
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return nullptr;
    }
    
    return db;
}

// Save page to database
bool savePage(sqlite3* db, const std::string& url, const std::string& content) {
    // Extract title
    std::string title = "Untitled";
    std::regex titleRegex("<title>([^<]+)</title>");
    std::smatch match;
    if (std::regex_search(content, match, titleRegex)) {
        title = match[1];
    }
    
    // Prepare SQL statement
    const char* insertSQL = "INSERT OR IGNORE INTO pages (url, title, content) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE);
}

class BasicCrawler {
private:
    std::queue<std::string> urlQueue;
    std::unordered_set<std::string> visitedUrls;
    std::mutex queueMutex;
    std::mutex dbMutex;
    std::atomic<bool> running{true};
    std::atomic<int> activeThreads{0};
    sqlite3* db;
    const int maxDepth;
    const int threadCount;

    // Callback function for CURL to write data
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Extract URLs from HTML
    std::vector<std::string> extractUrls(const std::string& html, const std::string& baseUrl) {
        std::vector<std::string> urls;
        std::regex linkRegex("<a\\s+(?:[^>]*?\\s+)?href=\"([^\"]*)\"", std::regex::icase);
        
        auto linksBegin = std::sregex_iterator(html.begin(), html.end(), linkRegex);
        auto linksEnd = std::sregex_iterator();

        for (std::sregex_iterator i = linksBegin; i != linksEnd; ++i) {
            std::smatch match = *i;
            std::string url = match[1].str();
            
            // Handle relative URLs
            if (url.find("http") != 0) {
                if (url[0] == '/') {
                    // Get domain from base URL
                    size_t pos = baseUrl.find("://");
                    if (pos != std::string::npos) {
                        pos += 3;
                        size_t endPos = baseUrl.find("/", pos);
                        if (endPos != std::string::npos) {
                            std::string domain = baseUrl.substr(0, endPos);
                            url = domain + url;
                        }
                    }
                } else {
                    // Handle relative path
                    size_t lastSlash = baseUrl.find_last_of('/');
                    if (lastSlash != std::string::npos) {
                        url = baseUrl.substr(0, lastSlash + 1) + url;
                    }
                }
            }
            
            urls.push_back(url);
        }

        return urls;
    }

    // Extract title from HTML
    std::string extractTitle(const std::string& html) {
        std::regex titleRegex("<title>([^<]*)</title>", std::regex::icase);
        std::smatch match;
        if (std::regex_search(html, match, titleRegex) && match.size() > 1) {
            return match[1].str();
        }
        return "No Title";
    }

    // Save page data to database
    void savePage(const std::string& url, const std::string& title, int depth) {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        sqlite3_stmt* stmt;
        const char* query = "INSERT OR IGNORE INTO pages (url, title, depth) VALUES (?, ?, ?)";
        
        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return;
        }
        
        sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, depth);
        
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Failed to insert data: " << sqlite3_errmsg(db) << std::endl;
        }
        
        sqlite3_finalize(stmt);
    }

    // Process a single URL
    void processUrl(const std::string& url, int depth) {
        if (depth > maxDepth) return;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Failed to initialize CURL" << std::endl;
            return;
        }
        
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "BasicWebCrawler/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            std::string title = extractTitle(response);
            std::cout << "Crawled: " << url << " (Depth: " << depth << ")" << std::endl;
            std::cout << "Title: " << title << std::endl;
            
            // Save to database
            savePage(url, title, depth);
            
            // Extract and queue new URLs
            if (depth < maxDepth) {
                std::vector<std::string> newUrls = extractUrls(response, url);
                std::lock_guard<std::mutex> lock(queueMutex);
                for (const auto& newUrl : newUrls) {
                    if (visitedUrls.find(newUrl) == visitedUrls.end()) {
                        urlQueue.push(newUrl);
                        visitedUrls.insert(newUrl);
                    }
                }
            }
        } else {
            std::cerr << "Failed to fetch URL: " << url << " - " << curl_easy_strerror(res) << std::endl;
        }
        
        curl_easy_cleanup(curl);
    }

    // Worker thread function
    void workerThread() {
        while (running) {
            std::string url;
            int depth = 0;
            
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (urlQueue.empty()) {
                    // No more URLs to process
                    break;
                }
                
                url = urlQueue.front();
                urlQueue.pop();
                
                // Check if we already know the depth for this URL
                sqlite3_stmt* stmt;
                const char* query = "SELECT depth FROM pages WHERE url = ?";
                
                {
                    std::lock_guard<std::mutex> dbLock(dbMutex);
                    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) == SQLITE_OK) {
                        sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC);
                        
                        if (sqlite3_step(stmt) == SQLITE_ROW) {
                            depth = sqlite3_column_int(stmt, 0) + 1;
                        }
                        
                        sqlite3_finalize(stmt);
                    }
                }
            }
            
            activeThreads++;
            processUrl(url, depth);
            activeThreads--;
        }
    }

public:
    BasicCrawler(const std::string& startUrl, int maxDepth = 3, int threadCount = 4) 
        : maxDepth(maxDepth), threadCount(threadCount) {
        
        // Initialize database
        if (sqlite3_open("crawler.db", &db) != SQLITE_OK) {
            std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            throw std::runtime_error("Database initialization failed");
        }
        
        // Create tables if they don't exist
        const char* createTableSQL = 
            "CREATE TABLE IF NOT EXISTS pages ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "url TEXT UNIQUE,"
            "title TEXT,"
            "depth INTEGER,"
            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
            ")";
            
        char* errMsg = nullptr;
        if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Failed to create table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            sqlite3_close(db);
            throw std::runtime_error("Table creation failed");
        }
        
        // Add start URL to queue
        urlQueue.push(startUrl);
        visitedUrls.insert(startUrl);
    }
    
    ~BasicCrawler() {
        if (db) {
            sqlite3_close(db);
        }
    }
    
    void start() {
        std::cout << "Starting crawler with " << threadCount << " threads..." << std::endl;
        
        std::vector<std::thread> threads;
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back(&BasicCrawler::workerThread, this);
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        std::cout << "Crawler finished. Total pages crawled: " << visitedUrls.size() << std::endl;
    }
    
    void stop() {
        running = false;
    }
    
    int getActiveThreadCount() const {
        return activeThreads;
    }
    
    int getQueueSize() const {
        std::lock_guard<std::mutex> lock(queueMutex);
        return urlQueue.size();
    }
};

int main(int argc, char* argv[]) {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    
    std::string startUrl = "https://en.wikipedia.org/wiki/Web_crawler";
    int maxDepth = 2;
    int threadCount = 4;
    
    if (argc > 1) {
        startUrl = argv[1];
    }
    
    if (argc > 2) {
        maxDepth = std::stoi(argv[2]);
    }
    
    if (argc > 3) {
        threadCount = std::stoi(argv[3]);
    }
    
    try {
        BasicCrawler crawler(startUrl, maxDepth, threadCount);
        
        // Register signal handler for Ctrl+C
        std::cout << "Press Ctrl+C to stop crawling..." << std::endl;
        
        crawler.start();
        
        // Print statistics
        std::cout << "Crawler finished successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    // Cleanup libcurl
    curl_global_cleanup();
    
    return 0;
} 