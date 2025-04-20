#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
// #include <tinyxml2.h>

// Forward declaration
namespace tinyxml2 {
    class XMLDocument;
}

// Forward declaration for CURL
typedef void CURL;
typedef int CURLcode;

class CrawlerFeatures {
public:
    struct RobotsRule {
        std::string userAgent;
        std::vector<std::string> allow;
        std::vector<std::string> disallow;
        int crawlDelay;
    };

    struct SitemapEntry {
        std::string url;
        std::chrono::system_clock::time_point lastModified;
        float priority;
        std::string changeFrequency;
    };

    CrawlerFeatures();
    ~CrawlerFeatures();

    // Content filtering
    bool isSupportedContentType(const std::string& contentType) const;
    bool shouldFollowLink(const std::string& url, const std::string& context) const;
    bool shouldIndexPage(const std::string& url, const std::string& content) const;
    bool isBlocked(const std::string& url, const std::string& userAgent) const;

    // Robots.txt handling
    bool loadRobotsTxt(const std::string& domain);
    bool isAllowed(const std::string& url, const std::string& userAgent = "*") const;
    int getCrawlDelay(const std::string& domain) const;

    // Sitemap handling
    bool loadSitemap(const std::string& url);
    std::vector<std::string> getSitemapUrls() const;
    std::vector<SitemapEntry> getSitemapEntries() const;

    // Additional methods
    std::string normalizeUrl(const std::string& url) const;
    std::string getContentType(const std::string& url) const;
    bool isBinaryContent(const std::string& contentType) const;

private:
    // Supported content types
    static const std::vector<std::string> SUPPORTED_CONTENT_TYPES;
    
    // Domain-specific settings
    std::unordered_map<std::string, std::vector<RobotsRule>> robotsRules;
    std::unordered_map<std::string, std::vector<SitemapEntry>> sitemaps;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> lastRobotsFetch;
    
    // CURL related
    CURL* curl;
    std::string userAgent;
    
    // Helper functions
    bool fetchRobotsTxt(const std::string& domain);
    bool parseRobotsTxt(const std::string& content, const std::string& domain);
    bool parseSitemap(const std::string& content);
    std::string extractDomain(const std::string& url) const;
    
    // Cache settings
    static constexpr std::chrono::hours ROBOTS_CACHE_DURATION{24};
}; 