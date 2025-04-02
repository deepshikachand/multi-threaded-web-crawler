#include "crawler_features.hpp"
#include <stdexcept>
#include <regex>
#include <algorithm>
#include <tinyxml2.h>

const std::vector<std::string> CrawlerFeatures::SUPPORTED_CONTENT_TYPES = {
    "text/html",
    "text/plain",
    "application/xhtml+xml",
    "application/xml",
    "text/xml",
    "application/rss+xml",
    "application/atom+xml",
    "application/json",
    "application/javascript",
    "text/css"
};

CrawlerFeatures::CrawlerFeatures() : userAgent("WebCrawler/1.0") {
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

CrawlerFeatures::~CrawlerFeatures() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
}

bool CrawlerFeatures::loadRobotsTxt(const std::string& domain) {
    auto now = std::chrono::system_clock::now();
    auto it = lastRobotsFetch.find(domain);
    
    if (it != lastRobotsFetch.end() && 
        now - it->second < ROBOTS_CACHE_DURATION) {
        return true;
    }

    if (!fetchRobotsTxt(domain)) {
        return false;
    }

    lastRobotsFetch[domain] = now;
    return true;
}

bool CrawlerFeatures::isAllowed(const std::string& url, const std::string& userAgent) const {
    std::string domain = extractDomain(url);
    auto it = robotsRules.find(domain);
    
    if (it == robotsRules.end()) {
        return true; // Default allow if no rules found
    }

    for (const auto& rule : it->second) {
        if (rule.userAgent == "*" || rule.userAgent == userAgent) {
            for (const auto& disallow : rule.disallow) {
                if (url.find(disallow) != std::string::npos) {
                    return false;
                }
            }
            for (const auto& allow : rule.allow) {
                if (url.find(allow) != std::string::npos) {
                    return true;
                }
            }
        }
    }

    return true;
}

int CrawlerFeatures::getCrawlDelay(const std::string& domain) const {
    auto it = robotsRules.find(domain);
    if (it != robotsRules.end()) {
        for (const auto& rule : it->second) {
            if (rule.userAgent == "*") {
                return rule.crawlDelay;
            }
        }
    }
    return 0; // Default no delay
}

bool CrawlerFeatures::loadSitemap(const std::string& url) {
    std::string content;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, 
        [](void* contents, size_t size, size_t nmemb, std::string* userp) {
            userp->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return false;
    }

    return parseSitemap(content);
}

std::vector<std::string> CrawlerFeatures::getSitemapUrls() const {
    std::vector<std::string> urls;
    for (const auto& sitemap : sitemaps) {
        for (const auto& entry : sitemap.second) {
            urls.push_back(entry.url);
        }
    }
    return urls;
}

std::vector<CrawlerFeatures::SitemapEntry> CrawlerFeatures::getSitemapEntries() const {
    std::vector<SitemapEntry> entries;
    for (const auto& sitemap : sitemaps) {
        entries.insert(entries.end(), sitemap.second.begin(), sitemap.second.end());
    }
    return entries;
}

bool CrawlerFeatures::isSupportedContentType(const std::string& contentType) const {
    return std::find(SUPPORTED_CONTENT_TYPES.begin(), 
                    SUPPORTED_CONTENT_TYPES.end(), 
                    contentType) != SUPPORTED_CONTENT_TYPES.end();
}

std::string CrawlerFeatures::getContentType(const std::string& url) const {
    std::string content;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, 
        [](void* contents, size_t size, size_t nmemb, std::string* userp) {
            userp->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return "";
    }

    std::regex contentTypeRegex("Content-Type:\\s*([^\\r\\n]+)");
    std::smatch match;
    if (std::regex_search(content, match, contentTypeRegex)) {
        return match[1].str();
    }
    return "";
}

bool CrawlerFeatures::isBinaryContent(const std::string& contentType) const {
    return contentType.find("text/") == std::string::npos &&
           contentType.find("application/json") == std::string::npos &&
           contentType.find("application/xml") == std::string::npos;
}

bool CrawlerFeatures::shouldFollowLink(const std::string& url, const std::string& contentType) const {
    if (isBinaryContent(contentType)) {
        return false;
    }
    return isAllowed(url);
}

std::string CrawlerFeatures::normalizeUrl(const std::string& url) const {
    std::string normalized = url;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove trailing slash
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    return normalized;
}

bool CrawlerFeatures::fetchRobotsTxt(const std::string& domain) {
    std::string url = "https://" + domain + "/robots.txt";
    std::string content;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, 
        [](void* contents, size_t size, size_t nmemb, std::string* userp) {
            userp->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        return false;
    }

    return parseRobotsTxt(content, domain);
}

bool CrawlerFeatures::parseRobotsTxt(const std::string& content, const std::string& domain) {
    std::vector<RobotsRule> rules;
    RobotsRule currentRule;
    currentRule.userAgent = "*";
    currentRule.crawlDelay = 0;

    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string directive;
        lineStream >> directive;

        if (directive == "User-agent:") {
            if (!currentRule.allow.empty() || !currentRule.disallow.empty()) {
                rules.push_back(currentRule);
                currentRule = RobotsRule();
            }
            lineStream >> currentRule.userAgent;
        }
        else if (directive == "Allow:") {
            std::string path;
            lineStream >> path;
            currentRule.allow.push_back(path);
        }
        else if (directive == "Disallow:") {
            std::string path;
            lineStream >> path;
            currentRule.disallow.push_back(path);
        }
        else if (directive == "Crawl-delay:") {
            lineStream >> currentRule.crawlDelay;
        }
    }

    if (!currentRule.allow.empty() || !currentRule.disallow.empty()) {
        rules.push_back(currentRule);
    }

    robotsRules[domain] = rules;
    return true;
}

bool CrawlerFeatures::parseSitemap(const std::string& content) {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(content.c_str()) != tinyxml2::XML_SUCCESS) {
        return false;
    }

    auto root = doc.RootElement();
    if (!root) {
        return false;
    }

    std::vector<SitemapEntry> entries;
    for (auto urlNode = root->FirstChildElement("url"); urlNode; 
         urlNode = urlNode->NextSiblingElement("url")) {
        SitemapEntry entry;
        
        auto loc = urlNode->FirstChildElement("loc");
        if (loc) {
            entry.url = loc->GetText();
        }

        auto lastmod = urlNode->FirstChildElement("lastmod");
        if (lastmod) {
            std::string lastmodStr = lastmod->GetText();
            // Parse ISO 8601 date
            std::tm tm = {};
            std::istringstream ss(lastmodStr);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            entry.lastModified = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }

        auto priority = urlNode->FirstChildElement("priority");
        if (priority) {
            entry.priority = std::stof(priority->GetText());
        }

        auto changefreq = urlNode->FirstChildElement("changefreq");
        if (changefreq) {
            entry.changeFrequency = changefreq->GetText();
        }

        entries.push_back(entry);
    }

    sitemaps[extractDomain(entries[0].url)] = entries;
    return true;
}

std::string CrawlerFeatures::extractDomain(const std::string& url) const {
    std::regex domainRegex("https?://([^/]+)");
    std::smatch match;
    if (std::regex_search(url, match, domainRegex)) {
        return match[1].str();
    }
    return "";
} 