#include "database.hpp"
#include <stdexcept>
#include <cstring>
#include <sys/stat.h>

Database::Database(const std::string& dbPath) : is_optimized(false) {
    // Open database with WAL mode
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    // Get file descriptor for database file
    db_fd = open(dbPath.c_str(), O_RDWR);
    if (db_fd == -1) {
        throw std::runtime_error("Failed to get database file descriptor");
    }

    // Set file permissions
    if (!setFilePermissions()) {
        throw std::runtime_error("Failed to set database file permissions");
    }

    // Enable WAL mode for better concurrency
    if (!enableWALMode()) {
        throw std::runtime_error("Failed to enable WAL mode");
    }

    // Initialize database
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize database");
    }

    // Prepare statements
    if (!prepareStatements()) {
        throw std::runtime_error("Failed to prepare statements");
    }
}

Database::~Database() {
    cleanupStatements();
    if (db) {
        sqlite3_close(db);
    }
    if (db_fd != -1) {
        close(db_fd);
    }
}

bool Database::setFilePermissions() {
    // Set read/write permissions for owner, read for others
    return fchmod(db_fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == 0;
}

bool Database::enableWALMode() {
    return executeQuery("PRAGMA journal_mode=WAL;");
}

bool Database::initialize() {
    const char* create_tables[] = {
        "CREATE TABLE IF NOT EXISTS pages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "url TEXT UNIQUE NOT NULL,"
        "title TEXT,"
        "content TEXT,"
        "domain TEXT,"
        "depth INTEGER,"
        "last_crawled TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "is_indexed BOOLEAN DEFAULT 0,"
        "error_count INTEGER DEFAULT 0"
        ");",
        
        "CREATE TABLE IF NOT EXISTS url_queue ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "url TEXT UNIQUE NOT NULL,"
        "depth INTEGER,"
        "priority INTEGER DEFAULT 0,"
        "status TEXT DEFAULT 'pending',"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "processed_at TIMESTAMP"
        ");",
        
        "CREATE INDEX IF NOT EXISTS idx_url_queue_status ON url_queue(status);",
        "CREATE INDEX IF NOT EXISTS idx_url_queue_priority ON url_queue(priority DESC);",
        "CREATE INDEX IF NOT EXISTS idx_pages_domain ON pages(domain);"
    };

    for (const char* query : create_tables) {
        if (!executeQuery(query)) {
            return false;
        }
    }

    return true;
}

bool Database::prepareStatements() {
    const char* insert_page = 
        "INSERT OR REPLACE INTO pages (url, title, content, domain, depth) "
        "VALUES (?, ?, ?, ?, ?);";
    
    const char* check_page = 
        "SELECT 1 FROM pages WHERE url = ?;";
    
    const char* update_error = 
        "UPDATE pages SET error_count = error_count + 1 WHERE url = ?;";
    
    const char* insert_url = 
        "INSERT OR IGNORE INTO url_queue (url, depth, priority) VALUES (?, ?, ?);";
    
    const char* get_url = 
        "SELECT url, depth FROM url_queue "
        "WHERE status = 'pending' "
        "ORDER BY priority DESC, created_at ASC LIMIT 1;";
    
    const char* update_url = 
        "UPDATE url_queue SET status = ?, processed_at = CURRENT_TIMESTAMP "
        "WHERE url = ?;";

    return sqlite3_prepare_v2(db, insert_page, -1, &insert_page_stmt, nullptr) == SQLITE_OK &&
           sqlite3_prepare_v2(db, check_page, -1, &check_page_stmt, nullptr) == SQLITE_OK &&
           sqlite3_prepare_v2(db, update_error, -1, &update_error_stmt, nullptr) == SQLITE_OK &&
           sqlite3_prepare_v2(db, insert_url, -1, &insert_url_stmt, nullptr) == SQLITE_OK &&
           sqlite3_prepare_v2(db, get_url, -1, &get_url_stmt, nullptr) == SQLITE_OK &&
           sqlite3_prepare_v2(db, update_url, -1, &update_url_stmt, nullptr) == SQLITE_OK;
}

void Database::cleanupStatements() {
    sqlite3_finalize(insert_page_stmt);
    sqlite3_finalize(check_page_stmt);
    sqlite3_finalize(update_error_stmt);
    sqlite3_finalize(insert_url_stmt);
    sqlite3_finalize(get_url_stmt);
    sqlite3_finalize(update_url_stmt);
}

bool Database::executeQuery(const std::string& query) {
    std::unique_lock<std::shared_mutex> lock(db_mutex);
    char* errMsg = nullptr;
    
    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) {
            sqlite3_free(errMsg);
        }
        return false;
    }
    return true;
}

bool Database::addPage(const std::string& url, const std::string& title,
                      const std::string& content, const std::string& domain, int depth) {
    std::unique_lock<std::shared_mutex> lock(db_mutex);
    
    sqlite3_bind_text(insert_page_stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_page_stmt, 2, title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_page_stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_page_stmt, 4, domain.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(insert_page_stmt, 5, depth);
    
    return sqlite3_step(insert_page_stmt) == SQLITE_DONE;
}

bool Database::isPageVisited(const std::string& url) {
    std::shared_lock<std::shared_mutex> lock(db_mutex);
    
    sqlite3_bind_text(check_page_stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    return sqlite3_step(check_page_stmt) == SQLITE_ROW;
}

bool Database::updatePageErrorCount(const std::string& url) {
    std::unique_lock<std::shared_mutex> lock(db_mutex);
    
    sqlite3_bind_text(update_error_stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    return sqlite3_step(update_error_stmt) == SQLITE_DONE;
}

bool Database::addUrlToQueue(const std::string& url, int depth, int priority) {
    std::unique_lock<std::shared_mutex> lock(db_mutex);
    
    sqlite3_bind_text(insert_url_stmt, 1, url.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(insert_url_stmt, 2, depth);
    sqlite3_bind_int(insert_url_stmt, 3, priority);
    
    return sqlite3_step(insert_url_stmt) == SQLITE_DONE;
}

bool Database::getNextUrl(std::string& url, int& depth) {
    std::unique_lock<std::shared_mutex> lock(db_mutex);
    
    if (sqlite3_step(get_url_stmt) == SQLITE_ROW) {
        url = reinterpret_cast<const char*>(sqlite3_column_text(get_url_stmt, 0));
        depth = sqlite3_column_int(get_url_stmt, 1);
        return true;
    }
    return false;
}

bool Database::markUrlAsProcessed(const std::string& url, const std::string& status) {
    std::unique_lock<std::shared_mutex> lock(db_mutex);
    
    sqlite3_bind_text(update_url_stmt, 1, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(update_url_stmt, 2, url.c_str(), -1, SQLITE_STATIC);
    
    return sqlite3_step(update_url_stmt) == SQLITE_DONE;
}

void Database::optimize() {
    if (!is_optimized) {
        executeQuery("PRAGMA optimize;");
        is_optimized = true;
    }
}

void Database::vacuum() {
    std::unique_lock<std::shared_mutex> lock(db_mutex);
    executeQuery("VACUUM;");
} 