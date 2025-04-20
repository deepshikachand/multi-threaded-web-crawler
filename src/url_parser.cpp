#include "../include/url_parser.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

URLParser::URLParser() {
#ifndef USE_STUB_IMPLEMENTATION
    // In real implementation, initialize curl
    curlHandle = nullptr;
#endif

    // Initialize regex patterns
    urlRegex = std::regex(R"(^(https?):\/\/([^\/\s]+)(\/[^\s]*)?(\?[^\s#]*)?(#[^\s]*)?$)");
    linkRegex = std::regex(R"(<a\s+[^>]*href=["']([^"']+)["'][^>]*>)");
    imageRegex = std::regex(R"(<img\s+[^>]*src=["']([^"']+)["'][^>]*>)");
}

URLParser::~URLParser() {
#ifndef USE_STUB_IMPLEMENTATION
    // In real implementation, cleanup curl
#endif
}

bool URLParser::parse(const std::string& url) {
    std::smatch match;
    if (std::regex_match(url, match, urlRegex)) {
        scheme = match[1].str();
        host = match[2].str();
        path = match[3].str().empty() ? "/" : match[3].str();
        query = match[4].str();
        fragment = match[5].str();
        return true;
    }
    return false;
}

std::string URLParser::normalize(const std::string& url) {
    // Basic URL normalization
    std::string result = url;
    
    // Convert to lowercase
    std::transform(result.begin(), result.end(), result.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    
    // Remove trailing slash if present
    if (!result.empty() && result.back() == '/') {
        result.pop_back();
    }
    
    // Remove fragments
    size_t fragmentPos = result.find('#');
    if (fragmentPos != std::string::npos) {
        result = result.substr(0, fragmentPos);
    }
    
    return result;
}

std::string URLParser::join(const std::string& baseUrl, const std::string& relativeUrl) {
    // Handle absolute URLs
    if (relativeUrl.find("http://") == 0 || relativeUrl.find("https://") == 0) {
        return relativeUrl;
    }
    
    // Parse base URL
    parse(baseUrl);
    std::string base = scheme + "://" + host;
    
    // Handle root-relative URLs
    if (!relativeUrl.empty() && relativeUrl[0] == '/') {
        return base + relativeUrl;
    }
    
    // Handle relative URLs
    std::string basePath = path;
    size_t lastSlash = basePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        basePath = basePath.substr(0, lastSlash + 1);
    } else {
        basePath = "/";
    }
    
    return base + basePath + relativeUrl;
}

std::string URLParser::getDomain(const std::string& url) {
    if (parse(url)) {
        return host;
    }
    return "";
}

bool URLParser::isValid(const std::string& url) {
    return std::regex_match(url, urlRegex);
}

std::vector<std::string> URLParser::extractLinks(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> links;
    std::sregex_iterator it(html.begin(), html.end(), linkRegex);
    std::sregex_iterator end;
    
    while (it != end) {
        std::string link = (*it)[1].str();
        // Join with base URL if relative
        if (link.find("http://") != 0 && link.find("https://") != 0) {
            link = join(baseUrl, link);
        }
        links.push_back(link);
        ++it;
    }
    
    return links;
}

std::vector<std::string> URLParser::extractImages(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> images;
    std::sregex_iterator it(html.begin(), html.end(), imageRegex);
    std::sregex_iterator end;
    
    while (it != end) {
        std::string image = (*it)[1].str();
        // Join with base URL if relative
        if (image.find("http://") != 0 && image.find("https://") != 0) {
            image = join(baseUrl, image);
        }
        images.push_back(image);
        ++it;
    }
    
    return images;
}

int URLParser::getDepth(const std::string& url) {
    if (parse(url)) {
        // Count the number of path segments
        std::string pathCopy = path;
        if (pathCopy.empty() || pathCopy == "/") {
            return 0;
        }
        
        // Remove leading and trailing slashes
        if (pathCopy[0] == '/') {
            pathCopy = pathCopy.substr(1);
        }
        if (!pathCopy.empty() && pathCopy.back() == '/') {
            pathCopy.pop_back();
        }
        
        // Count segments
        int depth = 0;
        size_t pos = 0;
        while ((pos = pathCopy.find('/', pos)) != std::string::npos) {
            depth++;
            pos++;
        }
        
        // Add 1 for the last segment if path is not empty
        if (!pathCopy.empty()) {
            depth++;
        }
        
        return depth;
    }
    return 0;
}

std::string URLParser::getScheme() const {
    return scheme;
}

std::string URLParser::getHost() const {
    return host;
}

std::string URLParser::getPath() const {
    return path;
}

std::string URLParser::getQuery() const {
    return query;
}

std::string URLParser::getFragment() const {
    return fragment;
}

bool URLParser::shouldCrawl(const std::string& url, const std::vector<std::string>& allowedDomains) {
    // Check if URL is valid
    if (!isValid(url)) {
        return false;
    }
    
    // Parse URL to get domain
    std::string domain = getDomain(url);
    
    // If allowed domains list is empty, allow all
    if (allowedDomains.empty()) {
        return true;
    }
    
    // Check if domain is in allowed domains
    for (const auto& allowedDomain : allowedDomains) {
        if (domain == allowedDomain || 
            (domain.length() >= allowedDomain.length() + 1 &&
             domain.compare(domain.length() - allowedDomain.length() - 1, 
                            allowedDomain.length() + 1, 
                            "." + allowedDomain) == 0)) {
            return true;
        }
    }
    
    return false;
}
