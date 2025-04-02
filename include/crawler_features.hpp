#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <curl/curl.h>

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

    // Robots.txt handling
    bool loadRobotsTxt(const std::string& domain);
    bool isAllowed(const std::string& url, const std::string& userAgent = "*") const;
    int getCrawlDelay(const std::string& domain) const;

    // Sitemap handling
    bool loadSitemap(const std::string& url);
    std::vector<std::string> getSitemapUrls() const;
    std::vector<SitemapEntry> getSitemapEntries() const;

    // Content type handling
    bool isSupportedContentType(const std::string& contentType) const;
    std::string getContentType(const std::string& url) const;
    bool isBinaryContent(const std::string& contentType) const;

    // URL filtering
    bool shouldFollowLink(const std::string& url, const std::string& contentType) const;
    std::string normalizeUrl(const std::string& url) const;

private:
    std::unordered_map<std::string, std::vector<RobotsRule>> robotsRules;
    std::unordered_map<std::string, std::vector<SitemapEntry>> sitemaps;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> lastRobotsFetch;
    
    static constexpr std::chrono::hours ROBOTS_CACHE_DURATION{24};
    static const std::vector<std::string> SUPPORTED_CONTENT_TYPES;
    
    CURL* curl;
    std::string userAgent;
    
    bool fetchRobotsTxt(const std::string& domain);
    bool parseRobotsTxt(const std::string& content, const std::string& domain);
    bool parseSitemap(const std::string& content);
    std::string extractDomain(const std::string& url) const;
}; 