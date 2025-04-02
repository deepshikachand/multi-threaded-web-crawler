cmake_minimum_required(VERSION 3.10)
project(WebCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)

# Add compiler flags for Linux
if(UNIX)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add source files
set(SOURCES
    src/main.cpp
    src/crawler.cpp
    src/database.cpp
    src/url_parser.cpp
    src/file_indexer.cpp
    src/thread_pool.cpp
)

# Add header files
set(HEADERS
    include/crawler.hpp
    include/database.hpp
    include/url_parser.hpp
    include/file_indexer.hpp
    include/thread_pool.hpp
)

# Create executable
add_executable(webcrawler ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(webcrawler PRIVATE include)

# Link libraries
target_link_libraries(webcrawler PRIVATE
    CURL::libcurl
    SQLite::SQLite3
    Boost::system
    Boost::filesystem
    pthread
    dl
)

# Installation
install(TARGETS webcrawler
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
) 