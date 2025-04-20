#include "../include/universal_crawler.hpp"
#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <regex>
#include <sstream>
#include <random>
#include <filesystem>
#include <fstream>
#include <iomanip> // for std::put_time

namespace fs = std::filesystem;

// SimpleURLParser implementation
bool SimpleURLParser::isValidUrl(const std::string& url) {
    // More robust validation: URL should start with http:// or https://
    return url.size() > 8 && (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://");
}

std::string SimpleURLParser::extractDomain(const std::string& url) {
    // Extract domain from URL (improved version)
    size_t start = url.find("://");
    if (start == std::string::npos) return "";
    
    start += 3; // Skip ://
    size_t end = url.find('/', start);
    
    // Handle URLs without path
    if (end == std::string::npos) {
        // Check for query params or fragments
        end = url.find('?', start);
        if (end == std::string::npos) {
            end = url.find('#', start);
        }
        // If still no delimiter, use the entire rest of the URL
        if (end == std::string::npos) {
            return url.substr(start);
        }
    }
    
    return url.substr(start, end - start);
}

bool SimpleURLParser::parse(const std::string& url) {
    // Improved URL validation regex pattern
    std::regex urlPattern("^(https?://)?([^/:]+)(:[0-9]+)?(/.*)?$");
    std::smatch matches;
    
    if (!std::regex_match(url, matches, urlPattern)) {
        return false;
    }

    scheme = matches[1].length() > 0 ? matches[1].str().substr(0, matches[1].str().length() - 3) : "http";
    domain = matches[2].str();
    path = matches[4].length() > 0 ? matches[4].str() : "/";
    
    return true;
}

std::string SimpleURLParser::getDomain() const {
    return domain;
}

std::string SimpleURLParser::getPath() const {
    return path;
}

std::string SimpleURLParser::getScheme() const {
    return scheme;
}

// UniversalCrawler implementation
UniversalCrawler::UniversalCrawler() 
    : maxThreads(4),
      maxDepth(3),
      running(false),
      pagesCrawled(0),
      imagesSaved(0),
      activeThreads(0) {
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

UniversalCrawler::~UniversalCrawler() {
    stop();
}

void UniversalCrawler::setMaxThreads(int threads) {
    std::lock_guard<std::mutex> lock(mutex);
    maxThreads = std::max(1, threads);
}

void UniversalCrawler::setMaxDepth(int depth) {
    std::lock_guard<std::mutex> lock(mutex);
    maxDepth = std::max(0, depth);
}

void UniversalCrawler::setAllowedDomains(const std::vector<std::string>& domains) {
    std::lock_guard<std::mutex> lock(mutex);
    allowedDomains = domains;
}

void UniversalCrawler::start(const std::string& seedUrl, int depth) {
    // Don't start if already running
    if (isRunning()) {
        std::cout << "Crawler is already running, ignoring start request.\n";
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Reset state
    running = true;
    pagesCrawled = 0;
    imagesSaved = 0;
    activeThreads = 0;
    urlQueue = std::queue<std::pair<std::string, int>>();
    visitedUrls.clear();
    
    // Add seed URL to the queue
    if (SimpleURLParser::isValidUrl(seedUrl)) {
        urlQueue.push({seedUrl, depth});
        std::cout << "Added seed URL to queue: " << seedUrl << "\n";
    } else {
        std::cerr << "Error: Invalid seed URL: " << seedUrl << "\n";
        running = false;
        return;
    }
    
    // Create data directories if they don't exist
    ensureDirectoriesExist();
    
    // Start worker threads
    for (int i = 0; i < maxThreads; ++i) {
        threads.push_back(std::thread(&UniversalCrawler::crawlThread, this));
        std::cout << "Started worker thread " << i + 1 << "\n";
    }
    
    // Signal waiting threads
    queueCondition.notify_all();
}

void UniversalCrawler::ensureDirectoriesExist() {
    // Create necessary directories
    try {
        if (!fs::exists("data")) {
            fs::create_directory("data");
        }
        if (!fs::exists("data/images")) {
            fs::create_directory("data/images");
        }
        if (!fs::exists("data/content")) {
            fs::create_directory("data/content");
        }
        if (!fs::exists("logs")) {
            fs::create_directory("logs");
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating directories: " << e.what() << std::endl;
    }
}

void UniversalCrawler::stop() {
    if (!isRunning()) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(mutex);
        running = false;
        
        // Signal all waiting threads
        queueCondition.notify_all();
    }
    
    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Clear threads vector
    threads.clear();
    
    std::cout << "Crawler stopped. Processed " << pagesCrawled << " pages and found " 
              << imagesSaved << " images.\n";
}

int UniversalCrawler::getQueueSize() const {
    std::lock_guard<std::mutex> lock(mutex);
    return urlQueue.size();
}

int UniversalCrawler::getPagesCrawled() const {
    std::lock_guard<std::mutex> lock(mutex);
    return pagesCrawled;
}

int UniversalCrawler::getImagesSaved() const {
    std::lock_guard<std::mutex> lock(mutex);
    return imagesSaved;
}

int UniversalCrawler::getActiveThreads() const {
    std::lock_guard<std::mutex> lock(mutex);
    return activeThreads;
}

int UniversalCrawler::getUniqueUrls() const {
    std::lock_guard<std::mutex> lock(mutex);
    return visitedUrls.size();
}

bool UniversalCrawler::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex);
    return running;
}

void UniversalCrawler::crawlThread() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        activeThreads++;
    }
    
    while (true) {
        // Get URL from queue
        std::pair<std::string, int> urlEntry;
        bool hasUrl = false;
        
        {
            std::unique_lock<std::mutex> lock(mutex);
            
            // Wait for URL or stop signal with a timeout to prevent deadlocks
            auto waitResult = queueCondition.wait_for(lock, std::chrono::seconds(1), [this] {
                return !running || !urlQueue.empty();
            });
            
            // Check if we should exit
            if (!running && urlQueue.empty()) {
                break;
            }
            
            // If queue is empty (spurious wakeup or timeout), continue waiting
            if (urlQueue.empty()) {
                continue;
            }
            
            urlEntry = urlQueue.front();
            urlQueue.pop();
            hasUrl = true;
        }
        
        if (hasUrl) {
            try {
                // Process URL
                processUrl(urlEntry.first, urlEntry.second);
                
                // Mark as visited
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    visitedUrls.insert(urlEntry.first);
                    pagesCrawled++;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error processing URL " << urlEntry.first << ": " << e.what() << std::endl;
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(mutex);
        activeThreads--;
    }
}

void UniversalCrawler::processUrl(const std::string& url, int depth) {
    // Log the URL being processed
    {
        static std::mutex logMutex;
        std::lock_guard<std::mutex> lock(logMutex);
        std::ofstream logFile("logs/crawler.log", std::ios::app);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto timeT = std::chrono::system_clock::to_time_t(now);
            logFile << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S") 
                    << " [Thread " << std::this_thread::get_id() << "] "
                    << "Processing URL: " << url 
                    << " (depth: " << depth << ")" << std::endl;
            logFile.close();
        }
    }
    
    // Simulate crawling by sleeping for a random amount of time
    // Shorter sleep times for faster simulation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(50, 250);
    std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));
    
    std::cout << "Processed URL: " << url << " (depth: " << depth << ")" << std::endl;
    
    // Save page data to a simulated database
    savePageToDatabase(url, depth);
    
    // Save sample page content to file system
    savePageContent(url, depth);
    
    // Check if we need to continue crawling
    if (depth >= maxDepth) {
        return;
    }
    
    // Extract domain from current URL
    std::string currentDomain = SimpleURLParser::extractDomain(url);
    
    // Generate fake URLs that match the current domain
    std::vector<std::string> fakeUrls;
    int numUrls = 2 + (rand() % 6); // Generate 2-7 URLs per page (reduced for efficiency)
    
    for (int i = 0; i < numUrls; i++) {
        // Create URLs based on the domain pattern
        if (currentDomain.find("wikipedia.org") != std::string::npos) {
            // Generate Wikipedia-style paths
            generateWikipediaUrls(currentDomain, fakeUrls);
        } 
        else if (currentDomain.find("github.com") != std::string::npos) {
            // Generate GitHub-style paths
            generateGitHubUrls(currentDomain, fakeUrls);
        }
        else if (currentDomain.find("stackoverflow.com") != std::string::npos) {
            // Generate StackOverflow-style paths
            generateStackOverflowUrls(currentDomain, fakeUrls);
        }
        else {
            // Generic paths for other domains
            generateGenericUrls(currentDomain, fakeUrls);
        }
    }
    
    // Sometimes explore subdomains (20% chance - reduced from 25%)
    if (rand() % 5 == 0) {
        addSubdomains(currentDomain, fakeUrls);
    }
    
    // Add URLs to queue
    {
        std::lock_guard<std::mutex> lock(mutex);
        bool addedUrls = false;
        int addedCount = 0;
        
        for (const auto& fakeUrl : fakeUrls) {
            // Limit the number of URLs added per page to avoid queue explosion
            if (addedCount >= 5) break;
            
            // Skip invalid URLs
            if (!SimpleURLParser::isValidUrl(fakeUrl)) {
                continue;
            }
            
            // Check if URL is already visited or pending
            if (visitedUrls.find(fakeUrl) != visitedUrls.end()) {
                continue;
            }
            
            // Check if domain is allowed
            std::string domain = SimpleURLParser::extractDomain(fakeUrl);
            if (!allowedDomains.empty()) {
                bool domainAllowed = false;
                for (const auto& allowedDomain : allowedDomains) {
                    if (domain == allowedDomain || 
                        domain.find(allowedDomain) != std::string::npos ||
                        (domain.size() > allowedDomain.size() && 
                         domain.substr(domain.size() - allowedDomain.size()) == allowedDomain)) {
                        domainAllowed = true;
                        break;
                    }
                }
                
                if (!domainAllowed) {
                    continue;
                }
            }
            
            // Add URL to queue
            urlQueue.push({fakeUrl, depth + 1});
            addedCount++;
            addedUrls = true;
        }
        
        // Signal waiting threads if new URLs were added
        if (addedUrls) {
            queueCondition.notify_one();
        }
        
        // Simulate finding images (more likely on certain domains)
        int imageCount = simulateImageDiscovery(currentDomain);
        if (imageCount > 0) {
            imagesSaved += imageCount;
            
            // Save image metadata to database
            for (int i = 0; i < imageCount; i++) {
                saveImageMetadata(currentDomain, i);
            }
        }
    }
}

// Helper method to generate Wikipedia-style URLs
void UniversalCrawler::generateWikipediaUrls(const std::string& domain, std::vector<std::string>& urls) {
    // Extract language code (en, fr, de, etc.)
    std::string lang = domain.substr(0, domain.find("."));
    if (lang == "www" || lang == "wikipedia") lang = "en"; // Default to English
    
    // Wikipedia categories - reduced for better performance
    static const std::vector<std::string> categories = {
        "Science", "Technology", "Mathematics", "Computer_science", 
        "History", "Geography", "Arts", "Philosophy", "Religion"
    };
    
    // Wikipedia specific pages - reduced for better performance
    static const std::vector<std::string> specificPages = {
        "Albert_Einstein", "World_War_II", "COVID-19_pandemic", 
        "Artificial_intelligence", "Machine_learning", "Quantum_mechanics",
        "Web_crawler", "Internet", "Climate_change"
    };
    
    // Wikipedia portals & special pages - reduced for better performance
    static const std::vector<std::string> specialPages = {
        "Main_Page", "Portal:Contents", "Portal:Current_events", "Special:Random"
    };
    
    // Mix of different URL types
    int urlType = rand() % 10;
    std::string path;
    
    if (urlType < 3) { // 30% Category pages
        path = "/wiki/Category:" + categories[rand() % categories.size()];
    } 
    else if (urlType < 6) { // 30% Specific articles
        path = "/wiki/" + specificPages[rand() % specificPages.size()];
    }
    else if (urlType < 8) { // 20% Special/portal pages
        path = "/wiki/" + specialPages[rand() % specialPages.size()];
    }
    else { // 20% List pages
        path = "/wiki/List_of_" + categories[rand() % categories.size()];
    }
    
    urls.push_back("https://" + domain + path);
}

// Helper method to generate GitHub-style URLs
void UniversalCrawler::generateGitHubUrls(const std::string& domain, std::vector<std::string>& urls) {
    // Popular GitHub users/orgs - reduced for better performance
    static const std::vector<std::string> users = {
        "microsoft", "google", "facebook", "apple", "amazon", "netflix"
    };
    
    // Popular repos - reduced for better performance
    static const std::vector<std::string> repos = {
        "linux", "react", "tensorflow", "kubernetes", "angular", "vue"
    };
    
    // Github sections - reduced for better performance
    static const std::vector<std::string> sections = {
        "blob/master/README.md", "issues", "pulls", "wiki"
    };
    
    // Generate URL
    std::string user = users[rand() % users.size()];
    std::string repo = repos[rand() % repos.size()];
    std::string section = sections[rand() % sections.size()];
    
    urls.push_back("https://" + domain + "/" + user + "/" + repo);
    
    // Only add section sometimes to reduce URLs
    if (rand() % 2 == 0) {
        urls.push_back("https://" + domain + "/" + user + "/" + repo + "/" + section);
    }
}

// Helper method to generate StackOverflow-style URLs
void UniversalCrawler::generateStackOverflowUrls(const std::string& domain, std::vector<std::string>& urls) {
    // StackOverflow paths - reduced for better performance
    static const std::vector<std::string> paths = {
        "/questions/tagged/", "/questions/", "/users/"
    };
    
    // Popular tags - reduced for better performance
    static const std::vector<std::string> tags = {
        "javascript", "python", "java", "c++", "php", "html"
    };
    
    // Generate random question IDs
    int questionId = 1000000 + (rand() % 60000000);
    int userId = 100000 + (rand() % 9000000);
    
    // Generate URL
    int pathType = rand() % 10;
    
    if (pathType < 4) { // 40% Question
        urls.push_back("https://" + domain + "/questions/" + std::to_string(questionId));
    }
    else if (pathType < 7) { // 30% Tag
        std::string tag = tags[rand() % tags.size()];
        urls.push_back("https://" + domain + "/questions/tagged/" + tag);
    }
    else { // 30% User
        urls.push_back("https://" + domain + "/users/" + std::to_string(userId));
    }
}

// Helper method to generate generic URLs
void UniversalCrawler::generateGenericUrls(const std::string& domain, std::vector<std::string>& urls) {
    // Common paths for websites - reduced for better performance
    static const std::vector<std::string> commonPaths = {
        "/", "/about", "/contact", "/products", 
        "/blog", "/news", "/faq", "/support"
    };
    
    // Blog/news specific paths - reduced for better performance
    static const std::vector<std::string> blogPaths = {
        "/blog/post-" + std::to_string(1 + (rand() % 50)),
        "/news/article-" + std::to_string(1 + (rand() % 50))
    };
    
    // Product specific paths - reduced for better performance
    static const std::vector<std::string> productPaths = {
        "/products/category-" + std::to_string(1 + (rand() % 5)),
        "/services/solution-" + std::to_string(1 + (rand() % 3))
    };
    
    // Generate URL
    int pathType = rand() % 10;
    
    if (pathType < 5) { // 50% Common paths
        std::string path = commonPaths[rand() % commonPaths.size()];
        urls.push_back("https://" + domain + path);
    }
    else if (pathType < 8) { // 30% Blog/news
        std::string path = blogPaths[rand() % blogPaths.size()];
        urls.push_back("https://" + domain + path);
    }
    else { // 20% Product
        std::string path = productPaths[rand() % productPaths.size()];
        urls.push_back("https://" + domain + path);
    }
}

// Helper method to add subdomain variations
void UniversalCrawler::addSubdomains(const std::string& domain, std::vector<std::string>& urls) {
    // Common subdomains - reduced for better performance
    static const std::vector<std::string> commonSubdomains = {
        "blog", "shop", "support", "help", "api"
    };
    
    // Domain-specific subdomains
    if (domain.find("wikipedia.org") != std::string::npos) {
        // Language subdomains for Wikipedia
        static const std::vector<std::string> languages = {"en", "es", "de", "fr", "ru"};
        
        // Add just one language subdomain for efficiency
        std::string lang = languages[rand() % languages.size()];
        // Replace existing language subdomain if there is one
        size_t dotPos = domain.find(".");
        std::string baseDomain = domain.substr(dotPos + 1);
        urls.push_back("https://" + lang + "." + baseDomain + "/wiki/Main_Page");
    } 
    else {
        // Add just one common subdomain for regular sites
        std::string subdomain = commonSubdomains[rand() % commonSubdomains.size()];
        
        // Handle existing subdomains
        if (domain.find("www.") == 0) {
            // Replace www with new subdomain
            std::string baseDomain = domain.substr(4);
            urls.push_back("https://" + subdomain + "." + baseDomain + "/");
        } else if (domain.find(".") != std::string::npos) {
            // Add subdomain before first dot
            size_t dotPos = domain.find(".");
            std::string baseDomain = domain.substr(dotPos + 1);
            urls.push_back("https://" + subdomain + "." + baseDomain + "/");
        } else {
            // Just add subdomain if no dots
            urls.push_back("https://" + subdomain + "." + domain + "/");
        }
    }
}

// Helper method to simulate finding images
int UniversalCrawler::simulateImageDiscovery(const std::string& domain) {
    // Different domains have different likelihoods of containing images
    if (domain.find("wikipedia.org") != std::string::npos) {
        // Wikipedia pages usually have multiple images
        int chance = rand() % 100;
        if (chance < 85) { // 85% chance of images (reduced from 90%)
            return 1 + (rand() % 3); // 1-3 images (reduced from 1-4)
        }
    } 
    else if (domain.find("stackoverflow.com") != std::string::npos) {
        // StackOverflow has fewer images
        int chance = rand() % 100;
        if (chance < 25) { // 25% chance of images (reduced from 30%)
            return 1; // Usually just 1 image
        }
    }
    else if (domain.find("github.com") != std::string::npos) {
        // GitHub has a moderate number of images
        int chance = rand() % 100;
        if (chance < 40) { // 40% chance of images (reduced from 50%)
            return 1 + (rand() % 2); // 1-2 images
        }
    }
    else {
        // Generic site
        int chance = rand() % 100;
        if (chance < 60) { // 60% chance of images (reduced from 70%)
            return 1 + (rand() % 4); // 1-4 images (reduced from 1-6)
        }
    }
    
    return 0; // No images found
}

// Save page data to a simulated database (just writes to a CSV file)
void UniversalCrawler::savePageToDatabase(const std::string& url, int depth) {
    // Create data directory if it doesn't exist
    if (!fs::exists("data")) {
        fs::create_directory("data");
    }
    
    // Lock to prevent multiple threads from writing simultaneously
    static std::mutex fileMutex;
    std::lock_guard<std::mutex> lock(fileMutex);
    
    std::string filename = "data/crawled_pages.csv";
    bool fileExists = fs::exists(filename);
    bool isEmpty = false;
    
    if (fileExists) {
        std::ifstream checkFile(filename);
        isEmpty = checkFile.peek() == std::ifstream::traits_type::eof();
        checkFile.close();
    }
    
    std::ofstream file(filename, std::ios::app);
    if (file.is_open()) {
        // If new file or empty file, write header
        if (!fileExists || isEmpty) {
            file << "URL,Domain,Crawl_Depth,Timestamp\n";
        }
        
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        
        // Extract domain
        std::string domain = SimpleURLParser::extractDomain(url);
        
        // Write data
        file << url << "," 
             << domain << "," 
             << depth << "," 
             << timestamp << "\n";
        
        file.close();
    } else {
        std::cerr << "Error: Could not open database file for writing: " << filename << std::endl;
    }
}

// Save image metadata to a simulated database
void UniversalCrawler::saveImageMetadata(const std::string& domain, int imageIndex) {
    // Create data directory if it doesn't exist
    if (!fs::exists("data")) {
        fs::create_directory("data");
    }
    
    // Lock to prevent multiple threads from writing simultaneously
    static std::mutex fileMutex;
    std::lock_guard<std::mutex> lock(fileMutex);
    
    std::string filename = "data/discovered_images.csv";
    bool fileExists = fs::exists(filename);
    bool isEmpty = false;
    
    if (fileExists) {
        std::ifstream checkFile(filename);
        isEmpty = checkFile.peek() == std::ifstream::traits_type::eof();
        checkFile.close();
    }
    
    std::ofstream file(filename, std::ios::app);
    if (file.is_open()) {
        // If new file or empty file, write header
        if (!fileExists || isEmpty) {
            file << "Domain,Image_URL,Image_Type,Size_KB,Timestamp\n";
        }
        
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        
        // Generate fake image URL with improved naming
        std::string imageName = "img_" + std::to_string(imageIndex) + "_" + 
                               std::to_string(rand() % 10000);
        std::string imageUrl = "https://" + domain + "/images/" + imageName;
        
        // Random image type and size
        static const std::vector<std::string> imageTypes = {"jpg", "png", "gif", "webp"};
        std::string imageType = imageTypes[rand() % imageTypes.size()];
        
        // Add the image extension to the URL
        imageUrl += "." + imageType;
        
        // More realistic file size distribution
        int sizeKB;
        if (imageType == "webp" || imageType == "png") {
            // WebP and PNG tend to be smaller
            sizeKB = 10 + (rand() % 500); 
        } else if (imageType == "gif") {
            // GIFs can vary widely
            sizeKB = 5 + (rand() % 1000);
        } else {
            // JPGs tend to be larger
            sizeKB = 20 + (rand() % 1000);
        }
        
        // Write data
        file << domain << "," 
             << imageUrl << "," 
             << imageType << "," 
             << sizeKB << "," 
             << timestamp << "\n";
        
        file.close();
        
        // Simulate saving image to filesystem
        saveImageToFileSystem(domain, imageName, imageType, imageIndex);
    } else {
        std::cerr << "Error: Could not open image database file for writing: " << filename << std::endl;
    }
}

// New method to simulate saving images to file system
void UniversalCrawler::saveImageToFileSystem(const std::string& domain, 
                                          const std::string& imageName,
                                          const std::string& imageType,
                                          int imageIndex) {
    // Create image directory if it doesn't exist
    if (!fs::exists("data/images")) {
        fs::create_directory("data/images");
    }
    
    // Create domain-specific directory
    std::string domainDir = "data/images/" + domain;
    if (!fs::exists(domainDir)) {
        try {
            fs::create_directory(domainDir);
        } catch (const fs::filesystem_error& e) {
            // Handle domain names that may not be valid directory names
            // Replace invalid characters with underscores
            std::string safeDomain = domain;
            std::replace(safeDomain.begin(), safeDomain.end(), ':', '_');
            std::replace(safeDomain.begin(), safeDomain.end(), '*', '_');
            std::replace(safeDomain.begin(), safeDomain.end(), '?', '_');
            std::replace(safeDomain.begin(), safeDomain.end(), '"', '_');
            std::replace(safeDomain.begin(), safeDomain.end(), '<', '_');
            std::replace(safeDomain.begin(), safeDomain.end(), '>', '_');
            std::replace(safeDomain.begin(), safeDomain.end(), '|', '_');
            
            domainDir = "data/images/" + safeDomain;
            fs::create_directory(domainDir);
        }
    }
    
    // Create an empty file to simulate an image
    std::string imagePath = domainDir + "/" + imageName + "." + imageType;
    std::ofstream file(imagePath);
    if (file.is_open()) {
        // Write some mock data to simulate image content
        file << "This is a simulated image file for " << domain << " #" << imageIndex << "\n";
        file.close();
    }
}

// New method to save page content to filesystem
void UniversalCrawler::savePageContent(const std::string& url, int depth) {
    // Create data directory if it doesn't exist
    if (!fs::exists("data/content")) {
        fs::create_directory("data/content");
    }
    
    // Generate a filename from the URL
    std::string domain = SimpleURLParser::extractDomain(url);
    std::string filename = "data/content/" + domain + "_" + std::to_string(depth) + "_" + 
                           std::to_string(rand() % 10000) + ".html";
    
    // Lock to prevent multiple threads from writing simultaneously
    static std::mutex fileMutex;
    std::lock_guard<std::mutex> lock(fileMutex);
    
    std::ofstream file(filename);
    if (file.is_open()) {
        // Generate a simple mock HTML content
        file << "<!DOCTYPE html>\n<html>\n<head>\n";
        file << "  <title>Mock page for " << url << "</title>\n";
        file << "  <meta charset=\"UTF-8\">\n";
        file << "</head>\n<body>\n";
        file << "  <h1>Mock content for URL: " << url << "</h1>\n";
        file << "  <p>This is a simulated web page at depth " << depth << "</p>\n";
        file << "  <p>Domain: " << domain << "</p>\n";
        file << "  <hr>\n";
        file << "  <p>Generated by UniversalCrawler</p>\n";
        file << "</body>\n</html>";
        
        file.close();
    }
}
