#pragma once

#include "build_config.hpp"
#include "compat_fixes.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>

// Forward declaration
namespace nlohmann {
    class json;
}

/**
 * @class Config
 * @brief Configuration manager for the web crawler
 */
class Config {
public:
    Config(const std::string& configFile = "config.json");
    ~Config();
    
    /**
     * @brief Load configuration from file
     * @param configFile Path to configuration file
     * @return True if configuration was loaded successfully
     */
    bool loadConfig(const std::string& configFile);
    
    /**
     * @brief Save configuration to file
     * @param configFile Path to configuration file
     * @return True if configuration was saved successfully
     */
    bool saveConfig(const std::string& configFile = "");
    
    // Configuration getters
    std::string getStartUrl() const;
    int getMaxDepth() const;
    int getMaxPages() const;
    std::string getUserAgent() const;
    bool getRespectRobotsTxt() const;
    bool getFollowRedirects() const;
    int getTimeoutSeconds() const;
    int getRetryCount() const;
    
    // Thread settings
    int getThreadCount() const;
    int getQueueSizeLimit() const;
    int getBatchSize() const;
    
    // Storage settings
    std::string getDatabasePath() const;
    bool getSaveHtml() const;
    bool getSaveImages() const;
    std::string getImageDirectory() const;
    std::string getContentDirectory() const;
    
    // Filter settings
    const std::vector<std::string>& getAllowedDomains() const;
    const std::vector<std::string>& getAllowedPaths() const;
    const std::vector<std::string>& getExcludedPaths() const;
    const std::vector<std::string>& getAllowedExtensions() const;
    const std::vector<std::string>& getImageExtensions() const;
    
    // Logging settings
    std::string getLogFilePath() const;
    std::string getLogLevel() const;
    std::string getLogFile() const;
    bool getEnableConsoleOutput() const;
    int getStatusUpdateInterval() const;
    
private:
    void parseConfig();
    
    // Configuration data
    nlohmann::json configData;
    std::string configFilePath;
    
    // Default settings
    std::string startUrl = "https://example.com";
    int maxDepth = 3;
    int maxPages = 1000;
    std::string userAgent = "Mozilla/5.0 Multi-Threaded-Web-Crawler/1.0";
    bool respectRobotsTxt = true;
    bool followRedirects = true;
    int timeoutSeconds = 30;
    int retryCount = 3;
    
    // Thread settings
    int threadCount = 4;
    int queueSizeLimit = 10000;
    int batchSize = 100;
    
    // Storage settings
    std::string databasePath = "crawler_data.db";
    bool saveHtml = true;
    bool saveImages = true;
    std::string imageDirectory = "images";
    std::string contentDirectory = "content";
    
    // Filter settings
    std::vector<std::string> allowedDomains;
    std::vector<std::string> allowedPaths;
    std::vector<std::string> excludedPaths;
    std::vector<std::string> allowedExtensions;
    std::vector<std::string> imageExtensions;
    
    // Logging settings
    std::string logFilePath = "crawler.log";
    std::string logFile = "crawler.log";
    std::string logLevel = "INFO";
    bool enableConsoleOutput = true;
    int statusUpdateInterval = 5;
}; 