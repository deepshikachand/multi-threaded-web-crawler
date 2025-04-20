#pragma once

#ifdef __INTELLISENSE__

#include <string>
#include <vector>
#include <map>
#include <chrono>

// Add these methods to the Database class for IntelliSense only
class Database {
public:
    Database(const std::string& dbPath);
    ~Database();
    
    bool open();
    void close();
    
    bool addPage(const std::string& url, const std::string& title, 
                 const std::string& contentType, const std::string& content, 
                 int depth = 0);
    
    bool addPage(const std::string& url, const std::string& title);
    
    bool addUrl(const std::string& url, int depth, bool visited);
    
    struct ContentFeatures {
        std::string summary;
    };
    bool addContentFeatures(const std::string& url, const std::string& features);
    bool addContentFeatures(const std::string& url, const std::map<std::string, double>& features);
    
    // First variant of addImage with page URL and file path
    bool addImage(const std::string& url, const std::string& pageUrl, 
                 const std::string& filePath, const std::string& alt);
    
    // Second variant of addImage with description, labels and objects
    bool addImage(const std::string& url, const std::string& description,
                  const std::vector<std::string>& labels, 
                  const std::vector<std::string>& objects);
    
    bool imageExists(const std::string& url);
    std::string getImagePath(const std::string& url);
    
    bool hasUrl(const std::string& url) const;
    
    std::vector<std::string> getPageUrls() const;
    std::vector<std::string> getImageUrls() const;
    
    int getTotalPages() const;
    int getTotalImages() const;
    
    std::vector<std::pair<std::string, int>> getUnvisitedUrls(int limit);
    
    int getQueueSize() const;
};

class FileIndexer {
public:
    FileIndexer(const std::string& baseDir);
    
    bool savePage(const std::string& url, const std::string& content);
    std::string getPagePath(const std::string& url);
    
    bool saveImage(const std::string& url, const std::vector<uint8_t>& imageData, 
                   const std::string& extension);
};

class URLParser {
public:
    int getDepth(const std::string& url) const;
    std::string getDomain(const std::string& url) const;
    std::string extractTitle(const std::string& content) const;
    std::vector<std::string> extractLinks(const std::string& content, const std::string& baseUrl) const;
    std::vector<std::string> extractImages(const std::string& content, const std::string& baseUrl) const;
};

#endif // __INTELLISENSE__ 