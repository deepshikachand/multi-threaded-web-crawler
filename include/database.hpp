#pragma once

#include <string>
#include <memory>
#include <sqlite3.h>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>

class Database {
public:
    explicit Database(const std::string& dbPath);
    ~Database();

    // Initialize database tables with WAL mode for better concurrency
    bool initialize();

    // Page operations with optimistic locking
    bool addPage(const std::string& url, const std::string& title, 
                 const std::string& content, const std::string& domain, int depth);
    bool isPageVisited(const std::string& url);
    bool updatePageErrorCount(const std::string& url);

    // URL Queue operations with priority
    bool addUrlToQueue(const std::string& url, int depth, int priority = 0);
    bool getNextUrl(std::string& url, int& depth);
    bool markUrlAsProcessed(const std::string& url, const std::string& status = "completed");

    // Database optimization
    void optimize();
    void vacuum();

private:
    // Database connection with WAL mode
    sqlite3* db;
    
    // Synchronization primitives
    mutable std::shared_mutex db_mutex;
    std::atomic<bool> is_optimized;
    
    // File descriptor for database file
    int db_fd;
    
    // Helper functions
    bool executeQuery(const std::string& query);
    bool prepareStatements();
    void cleanupStatements();
    bool enableWALMode();
    bool setFilePermissions();
    
    // Prepared statements
    sqlite3_stmt* insert_page_stmt;
    sqlite3_stmt* check_page_stmt;
    sqlite3_stmt* update_error_stmt;
    sqlite3_stmt* insert_url_stmt;
    sqlite3_stmt* get_url_stmt;
    sqlite3_stmt* update_url_stmt;
}; 