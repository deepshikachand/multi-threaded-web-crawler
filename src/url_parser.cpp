#include "url_parser.hpp"
#include <stdexcept>
#include <algorithm>
#include <cctype>

// Initialize static regex patterns
const std::regex URLParser::url_pattern(
    R"(^(https?:\/\/)?([\da-z\.-]+)\.([a-z\.]{2,6})([\/\w \.-]*)*\/?$)",
    std::regex::icase
);

const std::regex URLParser::domain_pattern(
    R"(^([a-zA-Z0-9][a-zA-Z0-9-]{0,61}[a-zA-Z0-9]\.)+[a-zA-Z]{2,}$)"
);

const std::regex URLParser::url_extract_pattern(
    R"(href=["'](https?:\/\/[^"']+)["'])",
    std::regex::icase
);

URLParser::URLParser() {
    initializeCurl();
}

URLParser::~URLParser() {
    cleanupCurl();
}

void URLParser::initializeCurl() {
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

void URLParser::cleanupCurl() {
    if (curl) {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }
}

bool URLParser::isValidUrl(const std::string& url) {
    return std::regex_match(url, url_pattern);
}

std::string URLParser::normalizeUrl(const std::string& url) {
    std::string normalized = url;
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove trailing slash
    if (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    // Add http:// if no protocol specified
    if (normalized.find("://") == std::string::npos) {
        normalized = "http://" + normalized;
    }
    
    return normalized;
}

std::string URLParser::getDomain(const std::string& url) {
    std::smatch matches;
    if (std::regex_search(url, matches, domain_pattern)) {
        return matches[0];
    }
    return "";
}

std::string URLParser::getPath(const std::string& url) {
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        pos = url.find('/', pos + 3);
        if (pos != std::string::npos) {
            return url.substr(pos);
        }
    }
    return "/";
}

std::string URLParser::getProtocol(const std::string& url) {
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        return url.substr(0, pos);
    }
    return "http";
}

bool URLParser::isAllowedDomain(const std::string& url, const std::vector<std::string>& allowedDomains) {
    std::string domain = getDomain(url);
    return std::find(allowedDomains.begin(), allowedDomains.end(), domain) != allowedDomains.end();
}

bool URLParser::isSameDomain(const std::string& url1, const std::string& url2) {
    return getDomain(url1) == getDomain(url2);
}

std::vector<std::string> URLParser::extractUrls(const std::string& html) {
    std::vector<std::string> urls;
    std::sregex_iterator it(html.begin(), html.end(), url_extract_pattern);
    std::sregex_iterator end;
    
    for (; it != end; ++it) {
        std::smatch match = *it;
        urls.push_back(match[1]);
    }
    
    return urls;
}

bool URLParser::isWithinDepth(const std::string& url, int currentDepth, int maxDepth) {
    return currentDepth <= maxDepth;
}

std::string URLParser::encodeUrl(const std::string& url) {
    if (!curl) {
        throw std::runtime_error("CURL not initialized");
    }
    
    char* encoded = curl_easy_escape(curl, url.c_str(), url.length());
    if (!encoded) {
        throw std::runtime_error("Failed to encode URL");
    }
    
    std::string result(encoded);
    curl_free(encoded);
    return result;
}

std::string URLParser::decodeUrl(const std::string& url) {
    if (!curl) {
        throw std::runtime_error("CURL not initialized");
    }
    
    int decodedLength;
    char* decoded = curl_easy_unescape(curl, url.c_str(), url.length(), &decodedLength);
    if (!decoded) {
        throw std::runtime_error("Failed to decode URL");
    }
    
    std::string result(decoded, decodedLength);
    curl_free(decoded);
    return result;
}

std::string URLParser::escapeString(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length());
    
    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped += c;
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", static_cast<unsigned char>(c));
            escaped += hex;
        }
    }
    
    return escaped;
}

std::string URLParser::unescapeString(const std::string& str) {
    std::string unescaped;
    unescaped.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            char hex[3] = {str[i + 1], str[i + 2], '\0'};
            char c;
            if (sscanf(hex, "%02hhx", &c) == 1) {
                unescaped += c;
                i += 2;
                continue;
            }
        }
        unescaped += str[i];
    }
    
    return unescaped;
} 