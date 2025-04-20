#pragma once

#include "build_config.hpp"
#include <string>
#include <vector>
#include <regex>

class URLParser {
public:
    URLParser();
    ~URLParser();

    // Parse URL into components
    bool parse(const std::string& url);

    // Normalize URL
    std::string normalize(const std::string& url);

    // Join base URL with a relative URL
    std::string join(const std::string& baseUrl, const std::string& relativeUrl);

    // Extract domain from URL
    std::string getDomain(const std::string& url);

    // Check if URL is valid
    bool isValid(const std::string& url);

    // Extract links from HTML content
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl);

    // Extract images from HTML content
    std::vector<std::string> extractImages(const std::string& html, const std::string& baseUrl);

    // Get the depth of a URL (number of path segments)
    int getDepth(const std::string& url);

    // Get URL components
    std::string getScheme() const;
    std::string getHost() const;
    std::string getPath() const;
    std::string getQuery() const;
    std::string getFragment() const;

    // Filter URLs that should not be crawled
    bool shouldCrawl(const std::string& url, const std::vector<std::string>& allowedDomains);

private:
#ifndef USE_STUB_IMPLEMENTATION
    // Curl handle for real implementation
    void* curlHandle;
#endif

    // URL components
    std::string scheme;
    std::string host;
    std::string path;
    std::string query;
    std::string fragment;

    // Regex patterns for URL parsing
    std::regex urlRegex;
    std::regex linkRegex;
    std::regex imageRegex;
};
