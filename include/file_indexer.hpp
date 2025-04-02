#pragma once

#include <string>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace fs = std::filesystem;

class FileIndexer {
public:
    explicit FileIndexer(const std::string& baseDir);
    ~FileIndexer();
    
    // File operations with memory mapping
    bool savePage(const std::string& url, const std::string& content);
    bool loadPage(const std::string& url, std::string& content);
    bool deletePage(const std::string& url);
    
    // Indexing operations with memory mapping
    bool indexPage(const std::string& url, const std::string& content);
    std::vector<std::string> searchIndex(const std::string& query);
    
    // Directory management
    bool createDomainDirectory(const std::string& domain);
    std::string getPagePath(const std::string& url);
    
    // Index statistics
    size_t getTotalPages() const;
    size_t getPagesByDomain(const std::string& domain) const;

    // Memory management
    void flushIndex();
    void optimizeIndex();

private:
    // Base directory for storage
    fs::path base_directory;
    
    // Index data structures with memory mapping
    struct MappedFile {
        int fd;
        void* mapping;
        size_t size;
        ~MappedFile() {
            if (mapping) {
                munmap(mapping, size);
            }
            if (fd != -1) {
                close(fd);
            }
        }
    };
    
    std::unordered_map<std::string, std::unique_ptr<MappedFile>> url_to_file;
    std::unordered_map<std::string, size_t> domain_page_counts;
    
    // Thread safety
    mutable std::shared_mutex index_mutex;
    
    // Memory mapping settings
    static constexpr size_t MAPPING_SIZE = 1024 * 1024; // 1MB
    
    // Helper functions
    std::string sanitizeFilename(const std::string& url);
    bool createDirectory(const fs::path& path);
    bool mapFile(const std::string& path, MappedFile& mapped);
    void unmapFile(MappedFile& mapped);
    bool writeToMappedFile(MappedFile& mapped, const std::string& content);
    std::string readFromMappedFile(const MappedFile& mapped);
}; 