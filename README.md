# Multi-Threaded Web Crawler

A high-performance, multi-threaded web crawler implemented in C++ that can crawl websites, extract content and images, and store them locally.

## Key Features

- **Multi-threading**: Crawls multiple pages simultaneously for improved performance
- **Configurable crawling depth**: Control how deep the crawler traverses the website
- **Domain filtering**: Restrict crawling to specific domains
- **Content and image extraction**: Automatically saves HTML content and images
- **Database storage**: Stores crawled data in a SQLite database
- **Flexible configuration**: JSON-based configuration for easy customization

## Project Structure

The project is organized into the following directories:

- **src/**: Contains all source (.cpp) files
  - `crawler.cpp`: Main crawler implementation
  - `thread_pool.cpp`: Thread pool for parallel crawling
  - `url_parser.cpp`: URL parsing and normalization
  - `database.cpp`: Database operations
  - `file_indexer.cpp`: File storage operations
  - `image_analyzer.cpp`: Image processing
  - `monitoring.cpp`: Logging and statistics

- **include/**: Contains all header (.hpp) files
  - `crawler.hpp`: Main crawler interface
  - `thread_pool.hpp`: Thread pool interface
  - And others corresponding to implementation files

- **data/**: Storage for crawled content
  - `content/`: Saved HTML pages
  - `images/`: Saved images

- **logs/**: Log files for monitoring and debugging

## Getting Started

### Prerequisites

- Windows 10 or later
- C++17 compatible compiler (Visual Studio 2019 or later)
- CURL library for HTTP requests
- SQLite for database storage

### Running the Crawler

The simplest way to run the crawler is to use the provided `run_crawler.bat` script:

1. Double-click `run_crawler.bat`
2. Enter the website URL to crawl (e.g., https://example.com)
3. Specify the number of threads (default: 4)
4. Set the maximum crawl depth (default: 2)
5. Set the maximum pages to crawl (default: 50)
6. Confirm the settings

The crawler will then:
- Create a configuration file based on your parameters
- Start crawling the specified website
- Save HTML content to the `data/content` directory
- Save images to the `data/images` directory
- Store metadata in the SQLite database

### Using Custom Configuration

For advanced usage, you can create your own configuration file:

1. Copy `crawler_config.json` and modify it as needed
2. Run the crawler with your configuration: `wiki_crawler.exe your_config.json`

## Components Overview

### WebCrawler

The main crawler class that orchestrates the crawling process. It manages the URL queue, workers, and handles the crawling logic.

### ThreadPool

Manages a pool of worker threads for parallel crawling. It efficiently distributes the crawling workload across multiple threads.

### URLParser

Handles URL parsing, normalization, and validation to ensure proper traversal of web pages.

### Database

Stores metadata about crawled pages, including URLs, titles, and timestamps.

### FileIndexer

Handles saving HTML content and managing the file system storage.

### ImageAnalyzer

Processes and saves images found during crawling.

### Monitoring

Provides logging and statistics tracking during the crawling process.

## Configuration Options

The `crawler_config.json` file provides various configuration options, including:

- Starting URL
- Maximum crawl depth and pages
- Thread count
- Allowed domains and file extensions
- Logging settings
- And many more advanced options

## Building from Source

If you need to build the crawler from source:

1. Run one of the build scripts:
   - `build_simple.bat`: Builds a simplified version
   - `build_full.bat`: Builds the complete version with all features

2. The executable will be created in the project directory

## License

This project is licensed under the MIT License - see the LICENSE file for details. 