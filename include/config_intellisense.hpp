#pragma once

#ifdef __INTELLISENSE__

#include <string>
#include <vector>

// Config class for IntelliSense
class Config {
public:
    Config() {}
    ~Config() {}

    bool loadConfig(const std::string& configFile) { return true; }
    void parseConfig() {}

    // Crawler settings
    std::string getStartUrl() const { return "https://example.com"; }
    int getMaxDepth() const { return 3; }
    int getMaxPages() const { return 1000; }
    std::string getUserAgent() const { return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Multi-Threaded-Web-Crawler/1.0"; }
    bool getRespectRobotsTxt() const { return true; }
    bool getFollowRedirects() const { return true; }
    int getTimeoutSeconds() const { return 30; }
    int getRetryCount() const { return 3; }

    // Threading settings
    int getThreadCount() const { return 8; }
    int getQueueSizeLimit() const { return 10000; }
    int getBatchSize() const { return 20; }

    // Storage settings
    std::string getDatabasePath() const { return "data/crawler.db"; }
    bool getSaveHtml() const { return true; }
    bool getSaveImages() const { return true; }
    std::string getImageDirectory() const { return "data/images"; }
    std::string getContentDirectory() const { return "data/content"; }

    // Filter settings
    const std::vector<std::string>& getAllowedDomains() const { 
        static std::vector<std::string> domains = {"example.com"};
        return domains;
    }
    const std::vector<std::string>& getExcludedPaths() const { 
        static std::vector<std::string> paths = {"/private"};
        return paths;
    }
    const std::vector<std::string>& getAllowedExtensions() const { 
        static std::vector<std::string> exts = {".html", ".htm"};
        return exts;
    }
    const std::vector<std::string>& getImageExtensions() const { 
        static std::vector<std::string> exts = {".jpg", ".png"};
        return exts;
    }

    // Monitoring settings
    std::string getLogLevel() const { return "INFO"; }
    std::string getLogFile() const { return "logs/crawler.log"; }
    bool getEnableConsoleOutput() const { return true; }
    int getStatusUpdateInterval() const { return 5; }
};

#endif // __INTELLISENSE__ 