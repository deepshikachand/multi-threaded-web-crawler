#include "file_indexer.hpp"
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <cstring>
#include <shared_mutex>
#include <sstream>

#ifdef _WIN32
// Windows implementation
#include <windows.h>
#else
// Unix/Linux implementation
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

FileIndexer::FileIndexer(const std::string& baseDir) : base_directory(baseDir) {
    if (!fs::exists(base_directory)) {
        fs::create_directories(base_directory);
    }
}

FileIndexer::~FileIndexer() {
    flushIndex();
}

bool FileIndexer::createDirectory(const fs::path& path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
        return true;
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

std::string FileIndexer::sanitizeFilename(const std::string& url) {
    std::string sanitized = url;
    std::replace_if(sanitized.begin(), sanitized.end(),
        [](char c) { return !std::isalnum(c) && c != '-' && c != '_' && c != '.'; },
        '_');
    return sanitized;
}

bool FileIndexer::mapFile(const std::string& path, MappedFile& mapped) {
#ifdef _WIN32
    // Windows implementation
    mapped.hFile = CreateFileA(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (mapped.hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Set file size
    LARGE_INTEGER fileSize;
    fileSize.QuadPart = MAPPING_SIZE;
    if (!SetFilePointerEx(mapped.hFile, fileSize, NULL, FILE_BEGIN) ||
        !SetEndOfFile(mapped.hFile)) {
        CloseHandle(mapped.hFile);
        mapped.hFile = INVALID_HANDLE_VALUE;
        return false;
    }
    
    mapped.hMapping = CreateFileMapping(
        mapped.hFile,
        NULL,
        PAGE_READWRITE,
        0,
        MAPPING_SIZE,
        NULL
    );
    
    if (mapped.hMapping == NULL) {
        CloseHandle(mapped.hFile);
        mapped.hFile = INVALID_HANDLE_VALUE;
        return false;
    }
    
    mapped.mapping = MapViewOfFile(
        mapped.hMapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        MAPPING_SIZE
    );
    
    if (mapped.mapping == NULL) {
        CloseHandle(mapped.hMapping);
        CloseHandle(mapped.hFile);
        mapped.hMapping = NULL;
        mapped.hFile = INVALID_HANDLE_VALUE;
        return false;
    }
    
    mapped.size = MAPPING_SIZE;
    return true;
#else
    // Unix/Linux implementation
    mapped.fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (mapped.fd == -1) {
        return false;
    }

    // Ensure file is large enough
    if (ftruncate(mapped.fd, MAPPING_SIZE) == -1) {
        close(mapped.fd);
        mapped.fd = -1;
        return false;
    }

    mapped.mapping = mmap(nullptr, MAPPING_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                         mapped.fd, 0);
    if (mapped.mapping == MAP_FAILED) {
        close(mapped.fd);
        mapped.fd = -1;
        mapped.mapping = nullptr;
        return false;
    }

    mapped.size = MAPPING_SIZE;
    return true;
#endif
}

void FileIndexer::unmapFile(MappedFile& mapped) {
#ifdef _WIN32
    // Windows implementation
    if (mapped.mapping) {
        UnmapViewOfFile(mapped.mapping);
        mapped.mapping = NULL;
    }
    if (mapped.hMapping != NULL) {
        CloseHandle(mapped.hMapping);
        mapped.hMapping = NULL;
    }
    if (mapped.hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(mapped.hFile);
        mapped.hFile = INVALID_HANDLE_VALUE;
    }
#else
    // Unix/Linux implementation
    if (mapped.mapping) {
        munmap(mapped.mapping, mapped.size);
        mapped.mapping = nullptr;
    }
    if (mapped.fd != -1) {
        close(mapped.fd);
        mapped.fd = -1;
    }
#endif
}

bool FileIndexer::writeToMappedFile(MappedFile& mapped, const std::string& content) {
    if (!mapped.mapping || content.length() > mapped.size) {
        return false;
    }

    std::memcpy(mapped.mapping, content.c_str(), content.length());
    
#ifdef _WIN32
    // Windows implementation
    FlushViewOfFile(mapped.mapping, content.length());
#else
    // Unix/Linux implementation
    msync(mapped.mapping, content.length(), MS_SYNC);
#endif
    return true;
}

std::string FileIndexer::readFromMappedFile(const MappedFile& mapped) {
    if (!mapped.mapping) {
        return "";
    }

    return std::string(static_cast<char*>(mapped.mapping));
}

bool FileIndexer::savePage(const std::string& url, const std::string& content) {
    std::unique_lock<std::shared_mutex> lock(index_mutex);
    
    std::string filename = sanitizeFilename(url);
    fs::path filepath = base_directory / filename;
    
    auto& mapped = url_to_file[url];
    if (!mapped) {
        mapped = std::make_unique<MappedFile>();
#ifdef _WIN32
        mapped->hFile = INVALID_HANDLE_VALUE;
        mapped->hMapping = NULL;
#else
        mapped->fd = -1;
#endif
        mapped->mapping = nullptr;
    }
    
    if (!mapFile(filepath.string(), *mapped)) {
        return false;
    }
    
    return writeToMappedFile(*mapped, content);
}

bool FileIndexer::loadPage(const std::string& url, std::string& content) {
    std::shared_lock<std::shared_mutex> lock(index_mutex);
    
    auto it = url_to_file.find(url);
    if (it == url_to_file.end()) {
        return false;
    }
    
    content = readFromMappedFile(*it->second);
    return true;
}

bool FileIndexer::deletePage(const std::string& url) {
    std::unique_lock<std::shared_mutex> lock(index_mutex);
    
    auto it = url_to_file.find(url);
    if (it != url_to_file.end()) {
        std::string filename = sanitizeFilename(url);
        fs::path filepath = base_directory / filename;
        
        unmapFile(*it->second);
        url_to_file.erase(it);
        
        return fs::remove(filepath);
    }
    return false;
}

bool FileIndexer::indexPage(const std::string& url, const std::string& content) {
    std::unique_lock<std::shared_mutex> lock(index_mutex);
    
    std::string domain = url.substr(0, url.find_first_of("/"));
    if (!createDomainDirectory(domain)) {
        return false;
    }
    
    return savePage(url, content);
}

std::vector<std::string> FileIndexer::searchIndex(const std::string& query) {
    std::shared_lock<std::shared_mutex> lock(index_mutex);
    std::vector<std::string> results;
    
    for (const auto& [url, mapped] : url_to_file) {
        std::string content = readFromMappedFile(*mapped);
        if (content.find(query) != std::string::npos) {
            results.push_back(url);
        }
    }
    
    return results;
}

bool FileIndexer::createDomainDirectory(const std::string& domain) {
    fs::path domainPath = base_directory / domain;
    return createDirectory(domainPath);
}

std::string FileIndexer::getPagePath(const std::string& url) {
    std::string filename = sanitizeFilename(url);
    return (base_directory / filename).string();
}

size_t FileIndexer::getTotalPages() const {
    std::shared_lock<std::shared_mutex> lock(index_mutex);
    return url_to_file.size();
}

size_t FileIndexer::getPagesByDomain(const std::string& domain) const {
    std::shared_lock<std::shared_mutex> lock(index_mutex);
    return domain_page_counts.count(domain) ? domain_page_counts.at(domain) : 0;
}

void FileIndexer::flushIndex() {
    std::unique_lock<std::shared_mutex> lock(index_mutex);
    
    for (auto& [url, mapped] : url_to_file) {
        if (mapped && mapped->mapping) {
#ifdef _WIN32
            // Windows implementation
            FlushViewOfFile(mapped->mapping, mapped->size);
#else
            // Unix/Linux implementation
            msync(mapped->mapping, mapped->size, MS_SYNC);
#endif
        }
    }
}

void FileIndexer::optimizeIndex() {
    std::unique_lock<std::shared_mutex> lock(index_mutex);
    
    // Compact the index by removing unused space
    for (auto& [url, mapped] : url_to_file) {
        if (mapped && mapped->mapping) {
            unmapFile(*mapped);
        }
    }
    
    // Re-map files with optimized sizes
    for (auto& [url, mapped] : url_to_file) {
        if (mapped) {
            std::string path = getPagePath(url);
            mapFile(path, *mapped);
        }
    }
}

bool FileIndexer::saveImage(const std::string& url, const std::vector<uint8_t>& imageData, const std::string& extension) {
    std::unique_lock<std::shared_mutex> lock(index_mutex);
    
    try {
        // Create the images directory if it doesn't exist
        fs::path imagesDir = base_directory / "images";
        if (!fs::exists(imagesDir)) {
            fs::create_directories(imagesDir);
        }
        
        // Generate a filename from the URL and extension
        std::string baseFilename = sanitizeFilename(url);
        fs::path filePath = imagesDir / (baseFilename + "." + extension);
        
        // Write the binary data to the file
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());
        file.close();
        
        // Store the mapping from URL to file path
        image_paths[url] = filePath.string();
        
        return true;
    } catch (const std::exception&) {
        // Handle any exceptions
        return false;
    }
} 