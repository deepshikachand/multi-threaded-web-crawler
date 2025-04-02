#pragma once

#include <string>
#include <vector>
#include <regex>
#include <memory>
#include <curl/curl.h>

class URLParser {
public:
    URLParser();
    ~URLParser();

    // URL validation and normalization
    bool isValidUrl(const std::string& url);
    std::string normalizeUrl(const std::string& url);
    
    // URL parsing
    std::string getDomain(const std::string& url);
    std::string getPath(const std::string& url);
    std::string getProtocol(const std::string& url);
    
    // URL filtering
    bool isAllowedDomain(const std::string& url, const std::vector<std::string>& allowedDomains);
    bool isSameDomain(const std::string& url1, const std::string& url2);
    
    // URL extraction from HTML
    std::vector<std::string> extractUrls(const std::string& html);
    
    // URL depth checking
    bool isWithinDepth(const std::string& url, int currentDepth, int maxDepth);

    // URL encoding/decoding
    std::string encodeUrl(const std::string& url);
    std::string decodeUrl(const std::string& url);

private:
    // CURL handle for URL operations
    CURL* curl;
    
    // Regular expressions for URL parsing
    static const std::regex url_pattern;
    static const std::regex domain_pattern;
    static const std::regex url_extract_pattern;
    
    // Helper functions
    void initializeCurl();
    void cleanupCurl();
    std::string escapeString(const std::string& str);
    std::string unescapeString(const std::string& str);
}; 