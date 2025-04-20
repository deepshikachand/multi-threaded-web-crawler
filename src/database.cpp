#include "../include/database.hpp"
#include "../include/compat_fixes.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

// Use stub implementation for testing
#ifdef USE_STUB_IMPLEMENTATION

Database::Database(const std::string& dbPath) : dbPath(dbPath) {
    std::cout << "Using stub database implementation" << std::endl;
}

Database::~Database() {
    std::cout << "Closing stub database" << std::endl;
}

bool Database::initialize() {
    std::cout << "Initializing stub database" << std::endl;
    return true;
}

bool Database::addPage(const std::string& url, const std::string& title, 
                     const std::string& content, const std::string& filePath) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    pagesTitles[url] = title;
    pagesContent[url] = content;
    pagesFiles[url] = filePath;
    
    return true;
}

bool Database::pageExists(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    return pagesFiles.find(url) != pagesFiles.end();
}

std::string Database::getPageTitle(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = pagesTitles.find(url);
    return (it != pagesTitles.end()) ? it->second : "";
}

std::string Database::getPagePath(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = pagesFiles.find(url);
    return (it != pagesFiles.end()) ? it->second : "";
}

bool Database::addUrl(const std::string& url, int depth, bool visited) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    urlsDepth[url] = depth;
    urlsVisited[url] = visited;
    
    return true;
}

bool Database::markUrlVisited(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (urlsDepth.find(url) != urlsDepth.end()) {
        urlsVisited[url] = true;
        return true;
    }
    
    return false;
}

bool Database::urlExists(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    return urlsDepth.find(url) != urlsDepth.end();
}

bool Database::isUrlVisited(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = urlsVisited.find(url);
    return (it != urlsVisited.end()) ? it->second : false;
}

std::vector<std::pair<std::string, int>> Database::getUnvisitedUrls(int limit) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    std::vector<std::pair<std::string, int>> result;
    
    for (const auto& [url, depth] : urlsDepth) {
        auto visitedIt = urlsVisited.find(url);
        if (visitedIt == urlsVisited.end() || !visitedIt->second) {
            result.push_back({url, depth});
            if (result.size() >= static_cast<size_t>(limit)) {
                break;
            }
        }
    }
    
    return result;
}

bool Database::addImage(const std::string& url, const std::string& pageUrl, 
                      const std::string& filePath, const std::string& alt) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    imagesFiles[url] = filePath;
    
    return true;
}

bool Database::imageExists(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    return imagesFiles.find(url) != imagesFiles.end();
}

std::string Database::getImagePath(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = imagesFiles.find(url);
    return (it != imagesFiles.end()) ? it->second : "";
}

bool Database::addContentFeatures(const std::string& url, const std::map<std::string, double>& features) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    contentFeatures[url] = features;
    
    return true;
}

int Database::getQueueSize() {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    int count = 0;
    for (const auto& [url, visited] : urlsVisited) {
        if (!visited) {
            count++;
        }
    }
    
    return count;
}

#else
// Real implementation using SQLite

Database::Database(const std::string& dbPath) 
    : dbPath(dbPath)
    , isOpen(false)
    , totalPages(0)
    , totalImages(0) {
    
    // Create database directory if it doesn't exist
    size_t lastSlash = dbPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string dirPath = dbPath.substr(0, lastSlash);
        // Create directory (platform-specific implementation would go here)
    }
    
    // In minimal build, we don't actually open a real database
#ifndef MINIMAL_BUILD
    // Open database connection
    open();
#endif
}

Database::~Database() {
    // Close database connection if open
    if (isOpen) {
        close();
    }
}

bool Database::open() {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (isOpen) {
        return true;
    }
    
#ifndef MINIMAL_BUILD
    // Real database implementation would go here
#endif
    
    isOpen = true;
    return isOpen;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!isOpen) {
        return;
    }
    
#ifndef MINIMAL_BUILD
    // Real database implementation would go here
#endif
    
    isOpen = false;
}

bool Database::addPage(const std::string& url, const std::string& title, 
                       const std::string& contentType, const std::string& content, 
                       int depth) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!isOpen && !open()) {
        return false;
    }
    
#ifndef MINIMAL_BUILD
    // Real database implementation would go here
#else
    // Simplified implementation for minimal build
    pageUrls.push_back(url);
    pageTitles[url] = title;
    pageContentTypes[url] = contentType;
    pageDepths[url] = depth;
#endif
    
    totalPages++;
    return true;
}

bool Database::addImage(const std::string& url, const std::string& description,
                        const std::vector<std::string>& labels, 
                        const std::vector<std::string>& objects) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!isOpen && !open()) {
        return false;
    }
    
#ifndef MINIMAL_BUILD
    // Real database implementation would go here
#else
    // Simplified implementation for minimal build
    imageUrls.push_back(url);
    imageDescriptions[url] = description;
    imageLabels[url] = labels;
    imageObjects[url] = objects;
#endif
    
    totalImages++;
    return true;
}

bool Database::addContentFeatures(const std::string& url, const std::string& features) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!isOpen && !open()) {
        return false;
    }
    
#ifndef MINIMAL_BUILD
    // Real database implementation would go here
#else
    // Simplified implementation for minimal build
    contentFeatures[url] = features;
#endif
    
    return true;
}

std::vector<std::string> Database::getPageUrls() const {
    std::lock_guard<std::mutex> lock(dbMutex);
    return pageUrls;
}

std::vector<std::string> Database::getImageUrls() const {
    std::lock_guard<std::mutex> lock(dbMutex);
    return imageUrls;
}

int Database::getTotalPages() const {
    return totalPages;
}

int Database::getTotalImages() const {
    return totalImages;
}

bool Database::hasUrl(const std::string& url) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    for (const auto& pageUrl : pageUrls) {
        if (pageUrl == url) {
            return true;
        }
    }
    
    for (const auto& imageUrl : imageUrls) {
        if (imageUrl == url) {
            return true;
        }
    }
    
    return false;
}

int Database::getQueueSize() const {
    return 0; // This should be implemented in the real crawler
}

#endif
