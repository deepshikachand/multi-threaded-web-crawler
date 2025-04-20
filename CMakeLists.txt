cmake_minimum_required(VERSION 3.10)
project(WebCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use vcpkg for dependencies
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

# Option to use stub implementation instead of real dependencies
option(USE_STUB_IMPLEMENTATION "Use stub implementation without external dependencies" OFF)

# Windows specific configurations
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    # Disable specific warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Add compiler flags for Linux
if(UNIX)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Stub implementation to build without dependencies
if(USE_STUB_IMPLEMENTATION)
    add_definitions(-DSTUB_IMPLEMENTATION)
else()
    # Find required packages
    find_package(CURL REQUIRED)
    find_package(SQLite3 REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
    
    include_directories(${CURL_INCLUDE_DIRS})
    include_directories(${SQLite3_INCLUDE_DIRS})
endif()

# Add source files
set(SOURCES
    src/main.cpp
    src/crawler.cpp
    src/thread_pool.cpp
    src/url_parser.cpp
    src/database.cpp
    src/monitoring.cpp
    src/config.cpp
    src/universal_crawler.cpp
)

# Add header files
set(HEADERS
    include/crawler.hpp
    include/thread_pool.hpp
    include/url_parser.hpp
    include/database.hpp
    include/monitoring.hpp
    include/config.hpp
    include/curl_stubs.hpp
    include/sqlite_stubs.hpp
    include/universal_crawler.hpp
)

# Create executable
add_executable(webcrawler ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(webcrawler PRIVATE include)

# Link libraries
if(NOT USE_STUB_IMPLEMENTATION)
    target_link_libraries(webcrawler PRIVATE ${CURL_LIBRARIES} ${SQLite3_LIBRARIES} nlohmann_json::nlohmann_json)
endif()

# Installation
install(TARGETS webcrawler
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Copy configuration file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json COPYONLY)

# Create necessary directories
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/images)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/content)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/logs)