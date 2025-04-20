#pragma once

#include "build_config.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

#ifndef USE_STUB_IMPLEMENTATION
#include <sqlite3.h>
#endif

class Database {
public:
    Database(const std::string& dbPath);
    ~Database();

    // Initialize the database
    bool initialize();

    // Page operations
    bool addPage(const std::string& url, const std::string& title, 
                const std::string& content, const std::string& filePath);
    bool pageExists(const std::string& url);
    std::string getPageTitle(const std::string& url);
    std::string getPagePath(const std::string& url);

    // URL operations
    bool addUrl(const std::string& url, int depth, bool visited = false);
    bool markUrlVisited(const std::string& url);
    bool urlExists(const std::string& url);
    bool isUrlVisited(const std::string& url);
    std::vector<std::pair<std::string, int>> getUnvisitedUrls(int limit = 100);

    // Image operations
    bool addImage(const std::string& url, const std::string& pageUrl, 
                 const std::string& filePath, const std::string& alt = "");
    bool imageExists(const std::string& url);
    std::string getImagePath(const std::string& url);

    // Content features
    bool addContentFeatures(const std::string& url, const std::map<std::string, double>& features);

    // Queue operations
    int getQueueSize();

private:
#ifndef USE_STUB_IMPLEMENTATION
    sqlite3* db;
#endif
    std::string dbPath;
    std::mutex dbMutex;

    // Stub implementations for testing
#ifdef USE_STUB_IMPLEMENTATION
    std::map<std::string, std::string> pagesTitles;
    std::map<std::string, std::string> pagesContent;
    std::map<std::string, std::string> pagesFiles;
    std::map<std::string, bool> urlsVisited;
    std::map<std::string, int> urlsDepth;
    std::map<std::string, std::string> imagesFiles;
    std::map<std::string, std::map<std::string, double>> contentFeatures;
#endif
};
