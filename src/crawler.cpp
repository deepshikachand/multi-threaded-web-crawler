#include "crawler.hpp"
#include <stdexcept>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <regex>

WebCrawler::WebCrawler(const std::string& startUrl, size_t maxThreads, 
                      int maxDepth, const std::vector<std::string>& allowedDomains)
    : startUrl(startUrl)
    , maxThreads(maxThreads)
    , maxDepth(maxDepth)
    , allowedDomains(allowedDomains)
    , running(false)
    , pagesCrawled(0)
    , totalBytesDownloaded(0)
    , failedRequests(0)
    , averageResponseTime(0.0)
    , curl(nullptr) {

    // Initialize components
    threadPool = std::make_unique<ThreadPool>(maxThreads);
    database = std::make_unique<Database>("crawler.db");
    urlParser = std::make_unique<URLParser>();
    fileIndexer = std::make_unique<FileIndexer>("pages");
    resourceManager = std::make_unique<ResourceManager>(1024, 10); // 1GB RAM, 10GB disk
    crawlerFeatures = std::make_unique<CrawlerFeatures>();
    monitoring = std::make_unique<Monitoring>();
    contentAnalyzer = std::make_unique<ContentAnalyzer>();
    imageAnalyzer = std::make_unique<ImageAnalyzer>();

    // Initialize CURL
    initializeCurlHandles();

    monitoring->log(Monitoring::LogLevel::INFO, "WebCrawler initialized");
}

WebCrawler::~WebCrawler() {
    stop();
    cleanupCurlHandles();
    monitoring->log(Monitoring::LogLevel::INFO, "WebCrawler destroyed");
}

void WebCrawler::start() {
    if (running) {
        return;
    }

    running = true;
    monitoring->log(Monitoring::LogLevel::INFO, "Starting crawler");

    // Add start URL to queue
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        urlQueue.push_back(startUrl);
    }

    // Start processing URLs
    while (running) {
        std::string url;
        int depth = 0;

        // Get next URL from queue
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (urlQueue.empty()) {
                if (getActiveThreads() == 0) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            url = urlQueue.back();
            urlQueue.pop_back();
            depth = urlParser->getDepth(url);
        }

        // Check if we should crawl this URL
        if (!crawlerFeatures->shouldFollowLink(url, "")) {
            continue;
        }

        // Check rate limits
        std::string domain = urlParser->getDomain(url);
        if (!resourceManager->checkRateLimit(domain)) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // Submit crawling task to thread pool
        threadPool->submit([this, url, depth]() {
            monitoring->startProfiling("crawl_page");
            crawlPage(url, depth);
            monitoring->stopProfiling("crawl_page");
        });
    }

    monitoring->log(Monitoring::LogLevel::INFO, "Crawler finished");
}

void WebCrawler::stop() {
    if (!running) {
        return;
    }

    running = false;
    monitoring->log(Monitoring::LogLevel::INFO, "Stopping crawler");
}

void WebCrawler::crawlPage(const std::string& url, int depth) {
    if (depth > maxDepth) {
        return;
    }

    CURL* handle = getCurlHandle();
    if (!handle) {
        failedRequests++;
        return;
    }

    std::string content;
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &content);
    curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, progressCallback);
    curl_easy_setopt(handle, CURLOPT_XFERINFODATA, this);

    auto startTime = std::chrono::steady_clock::now();
    CURLcode res = curl_easy_perform(handle);
    auto endTime = std::chrono::steady_clock::now();

    if (res != CURLE_OK) {
        failedRequests++;
        monitoring->log(Monitoring::LogLevel::ERROR, 
            "Failed to fetch " + url + ": " + curl_easy_strerror(res));
    } else {
        long httpCode;
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &httpCode);
        
        if (httpCode == 200) {
            std::string contentType;
            curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &contentType);
            
            if (crawlerFeatures->isSupportedContentType(contentType)) {
                processPage(url, content);
                pagesCrawled++;
            }
        } else {
            failedRequests++;
            monitoring->log(Monitoring::LogLevel::WARNING,
                "HTTP " + std::to_string(httpCode) + " for " + url);
        }
    }

    // Update average response time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    double newAvg = (averageResponseTime * pagesCrawled + duration) / (pagesCrawled + 1);
    averageResponseTime = newAvg;

    releaseCurlHandle(handle);
}

void WebCrawler::processPage(const std::string& url, const std::string& content) {
    monitoring->startProfiling("process_page");

    // Check if this is an image URL
    if (isImageUrl(url)) {
        std::vector<uint8_t> imageData(content.begin(), content.end());
        processImage(url, imageData);
        return;
    }

    // Extract title and save to database
    std::string title = urlParser->extractTitle(content);
    database->addPage(url, title);

    // Save page content
    fileIndexer->savePage(url, content);

    // Analyze content
    auto features = contentAnalyzer->analyzeContent(content);
    database->addContentFeatures(url, features);

    // Extract and add new URLs
    addNewUrls(content, url, urlParser->getDepth(url));

    monitoring->stopProfiling("process_page");
}

void WebCrawler::processImage(const std::string& url, const std::vector<uint8_t>& imageData) {
    monitoring->startProfiling("process_image");

    try {
        // Analyze image
        auto features = imageAnalyzer->analyzeImageData(imageData);

        // Skip NSFW content
        if (features.isNSFW) {
            monitoring->log(Monitoring::LogLevel::WARNING, "Skipping NSFW image: " + url);
            return;
        }

        // Save image metadata to database
        database->addImage(url, features.description, features.labels, features.objects);

        // Save image content
        std::string extension = getImageExtension(url);
        fileIndexer->saveImage(url, imageData, extension);

        // Log analysis results
        monitoring->log(Monitoring::LogLevel::INFO, 
            "Processed image: " + url + "\n" + features.description);

    } catch (const std::exception& e) {
        monitoring->log(Monitoring::LogLevel::ERROR, 
            "Failed to process image " + url + ": " + e.what());
        failedRequests++;
    }

    monitoring->stopProfiling("process_image");
}

bool WebCrawler::isImageUrl(const std::string& url) {
    static const std::vector<std::string> imageExtensions = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".svg"
    };

    std::string lowerUrl = url;
    std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::tolower);

    for (const auto& ext : imageExtensions) {
        if (lowerUrl.find(ext) != std::string::npos) {
            return true;
        }
    }

    return false;
}

std::string WebCrawler::getImageExtension(const std::string& url) {
    static const std::regex extRegex("\\.([^.]+)$");
    std::smatch match;
    if (std::regex_search(url, match, extRegex)) {
        return match[1].str();
    }
    return "jpg"; // Default extension
}

void WebCrawler::addNewUrls(const std::string& content, const std::string& baseUrl, int depth) {
    monitoring->startProfiling("extract_urls");

    auto urls = urlParser->extractUrls(content);
    std::lock_guard<std::mutex> lock(queueMutex);

    for (const auto& url : urls) {
        std::string normalizedUrl = crawlerFeatures->normalizeUrl(url);
        
        if (visitedUrls.find(normalizedUrl) == visitedUrls.end()) {
            visitedUrls.insert(normalizedUrl);
            urlQueue.push_back(normalizedUrl);
        }
    }

    monitoring->stopProfiling("extract_urls");
}

void WebCrawler::initializeCurlHandles() {
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    curlHandles.resize(maxThreads);
    for (size_t i = 0; i < maxThreads; ++i) {
        curlHandles[i] = curl_easy_init();
        if (!curlHandles[i]) {
            throw std::runtime_error("Failed to initialize CURL handle " + std::to_string(i));
        }
    }
}

void WebCrawler::cleanupCurlHandles() {
    for (auto handle : curlHandles) {
        if (handle) {
            curl_easy_cleanup(handle);
        }
    }
    if (curl) {
        curl_easy_cleanup(curl);
    }
}

CURL* WebCrawler::getCurlHandle() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (curlHandles.empty()) {
        return nullptr;
    }
    CURL* handle = curlHandles.back();
    curlHandles.pop_back();
    return handle;
}

void WebCrawler::releaseCurlHandle(CURL* handle) {
    std::lock_guard<std::mutex> lock(queueMutex);
    curlHandles.push_back(handle);
}

size_t WebCrawler::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int WebCrawler::progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, 
                               curl_off_t ultotal, curl_off_t ulnow) {
    WebCrawler* crawler = static_cast<WebCrawler*>(clientp);
    if (dltotal > 0) {
        crawler->totalBytesDownloaded += dlnow;
    }
    return 0;
}

size_t WebCrawler::getPagesCrawled() const {
    return pagesCrawled;
}

size_t WebCrawler::getQueueSize() const {
    return database->getQueueSize();
}

size_t WebCrawler::getActiveThreads() const {
    return threadPool->getActiveThreadCount();
}

double WebCrawler::getAverageResponseTime() const {
    return averageResponseTime;
}

size_t WebCrawler::getTotalBytesDownloaded() const {
    return totalBytesDownloaded;
}

size_t WebCrawler::getFailedRequests() const {
    return failedRequests;
} 