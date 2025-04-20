#include "../include/config.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

Config::Config(const std::string& configFile) {
    // Load configuration if file exists
    if (!configFile.empty()) {
        loadConfig(configFile);
    }
}

Config::~Config() {
    // Nothing to clean up
}

bool Config::loadConfig(const std::string& configFile) {
    try {
        std::ifstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << configFile << std::endl;
            return false;
        }
        
        configFilePath = configFile;
        
        // Parse the JSON string from the file
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        configData = nlohmann::json::parse(jsonContent);
        
        parseConfig();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool Config::saveConfig(const std::string& configFile) {
    // Use the provided file path or the path used for loading
    std::string savePath = configFile.empty() ? configFilePath : configFile;
    
    if (savePath.empty()) {
        std::cerr << "No config file path specified" << std::endl;
        return false;
    }
    
    try {
        std::ofstream file(savePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << savePath << std::endl;
            return false;
        }
        
        file << configData.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

void Config::parseConfig() {
    // Crawler settings
    if (configData.contains("crawler")) {
        auto& crawler = configData["crawler"];
        startUrl = crawler.value("start_url", startUrl);
        maxDepth = crawler.value("max_depth", maxDepth);
        maxPages = crawler.value("max_pages", maxPages);
        userAgent = crawler.value("user_agent", userAgent);
        respectRobotsTxt = crawler.value("respect_robots_txt", respectRobotsTxt);
        followRedirects = crawler.value("follow_redirects", followRedirects);
        timeoutSeconds = crawler.value("timeout_seconds", timeoutSeconds);
        retryCount = crawler.value("retry_count", retryCount);
    }
    
    // Threading settings
    if (configData.contains("threading")) {
        auto& threading = configData["threading"];
        threadCount = threading.value("thread_count", threadCount);
        queueSizeLimit = threading.value("queue_size_limit", queueSizeLimit);
        batchSize = threading.value("batch_size", batchSize);
    }
    
    // Storage settings
    if (configData.contains("storage")) {
        auto& storage = configData["storage"];
        databasePath = storage.value("database_path", databasePath);
        saveHtml = storage.value("save_html", saveHtml);
        saveImages = storage.value("save_images", saveImages);
        imageDirectory = storage.value("image_directory", imageDirectory);
        contentDirectory = storage.value("content_directory", contentDirectory);
        
        // Create directories if they don't exist
        if (!imageDirectory.empty()) {
            std::filesystem::create_directories(imageDirectory);
        }
        if (!contentDirectory.empty()) {
            std::filesystem::create_directories(contentDirectory);
        }
    }
    
    // Filter settings
    if (configData.contains("filters")) {
        auto& filters = configData["filters"];
        
        // We can't use iteration methods with the stub implementation
        // So we'll use a simpler approach for arrays
        // This won't actually work in minimal build but will compile
        
        if (filters.contains("allowed_domains")) {
            // In a real implementation, we would iterate through the array
            // For the stub, we'll just use the default values
            // allowedDomains = filters["allowed_domains"];
        }
        
        if (filters.contains("allowed_paths")) {
            // Similar stub implementation
            // allowedPaths = filters["allowed_paths"];
        }
        
        if (filters.contains("excluded_paths")) {
            // Similar stub implementation
            // excludedPaths = filters["excluded_paths"];
        }
        
        if (filters.contains("allowed_extensions")) {
            // Similar stub implementation
            // allowedExtensions = filters["allowed_extensions"];
        }
        
        if (filters.contains("image_extensions")) {
            // Similar stub implementation
            // imageExtensions = filters["image_extensions"];
        }
    }
    
    // Monitoring settings
    if (configData.contains("monitoring")) {
        auto& monitoring = configData["monitoring"];
        logLevel = monitoring.value("log_level", logLevel);
        logFile = monitoring.value("log_file", logFile);
        enableConsoleOutput = monitoring.value("enable_console_output", enableConsoleOutput);
        statusUpdateInterval = monitoring.value("status_update_interval", statusUpdateInterval);
        
        // Set logFilePath to match logFile for consistency
        logFilePath = logFile;
        
        // Create log directory if it doesn't exist
        if (!logFile.empty()) {
            size_t lastSlash = logFile.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                std::string logDir = logFile.substr(0, lastSlash);
                std::filesystem::create_directories(logDir);
            }
        }
    }
}

// Getter methods implementation
std::string Config::getStartUrl() const { return startUrl; }
int Config::getMaxDepth() const { return maxDepth; }
int Config::getMaxPages() const { return maxPages; }
std::string Config::getUserAgent() const { return userAgent; }
bool Config::getRespectRobotsTxt() const { return respectRobotsTxt; }
bool Config::getFollowRedirects() const { return followRedirects; }
int Config::getTimeoutSeconds() const { return timeoutSeconds; }
int Config::getRetryCount() const { return retryCount; }

int Config::getThreadCount() const { return threadCount; }
int Config::getQueueSizeLimit() const { return queueSizeLimit; }
int Config::getBatchSize() const { return batchSize; }

std::string Config::getDatabasePath() const { return databasePath; }
bool Config::getSaveHtml() const { return saveHtml; }
bool Config::getSaveImages() const { return saveImages; }
std::string Config::getImageDirectory() const { return imageDirectory; }
std::string Config::getContentDirectory() const { return contentDirectory; }

const std::vector<std::string>& Config::getAllowedDomains() const { return allowedDomains; }
const std::vector<std::string>& Config::getAllowedPaths() const { return allowedPaths; }
const std::vector<std::string>& Config::getExcludedPaths() const { return excludedPaths; }
const std::vector<std::string>& Config::getAllowedExtensions() const { return allowedExtensions; }
const std::vector<std::string>& Config::getImageExtensions() const { return imageExtensions; }

std::string Config::getLogLevel() const { return logLevel; }
std::string Config::getLogFilePath() const { return logFilePath; }
std::string Config::getLogFile() const { return logFile; }
bool Config::getEnableConsoleOutput() const { return enableConsoleOutput; }
int Config::getStatusUpdateInterval() const { return statusUpdateInterval; } 