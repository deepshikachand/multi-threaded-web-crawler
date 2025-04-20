# Developer's Guide

This guide provides an overview of the Multi-Threaded Web Crawler's architecture and instructions for extending its functionality.

## Project Architecture

The crawler is built on a modular architecture with the following key components:

### Core Components

1. **WebCrawler**: The main crawler class that orchestrates the crawling process
2. **ThreadPool**: Manages worker threads for concurrent processing
3. **URLParser**: Extracts and normalizes URLs from HTML content
4. **Database**: Stores crawled pages, images, and metadata
5. **FileIndexer**: Handles file system operations for storing content
6. **Config**: Loads and manages configuration options

### Support Components

1. **ResourceManager**: Handles rate limiting and resource allocation
2. **CrawlerFeatures**: Provides URL filtering and robots.txt handling
3. **Monitoring**: Tracks performance metrics and logging
4. **ContentAnalyzer**: Extracts features from page content
5. **ImageAnalyzer**: Processes and analyzes downloaded images

## Code Structure

```
/include/               # Header files
  crawler.hpp           # Main crawler class definition
  thread_pool.hpp       # Thread pool implementation
  url_parser.hpp        # URL parsing and normalization
  database.hpp          # Database interface
  file_indexer.hpp      # File system operations
  config.hpp            # Configuration management
  resource_manager.hpp  # Rate limiting and resource allocation
  crawler_features.hpp  # URL filtering and robots.txt handling
  monitoring.hpp        # Performance tracking and logging
  content_analyzer.hpp  # Content analysis
  image_analyzer.hpp    # Image processing

/src/                   # Implementation files
  crawler.cpp           # WebCrawler implementation
  thread_pool.cpp       # ThreadPool implementation
  url_parser.cpp        # URLParser implementation
  database.cpp          # Database implementation
  file_indexer.cpp      # FileIndexer implementation
  config.cpp            # Config implementation
  resource_manager.cpp  # ResourceManager implementation
  crawler_features.cpp  # CrawlerFeatures implementation
  monitoring.cpp        # Monitoring implementation
  content_analyzer.cpp  # ContentAnalyzer implementation
  image_analyzer.cpp    # ImageAnalyzer implementation
  main.cpp              # Program entry point
```

## Key Design Patterns

The crawler uses several design patterns:

1. **Factory Pattern**: Creating various handlers based on content type
2. **Strategy Pattern**: For different URL parsing strategies
3. **Observer Pattern**: For monitoring and event notifications
4. **Singleton Pattern**: For resource managers and the database connection

## Threading Model

The crawler uses a thread pool with worker threads to process URLs concurrently:

1. The main thread maintains the URL queue and dispatches work
2. Worker threads handle HTTP requests and process content
3. Synchronization is managed through mutexes on shared resources
4. Results are written to the database with appropriate locking

## Adding New Features

### Extending URL Filtering

To add new URL filtering capabilities:

1. Modify `CrawlerFeatures::shouldFollowLink()` in `crawler_features.cpp`
2. Add your custom logic to determine if a URL should be crawled
3. Update any configuration options in `config.hpp` and `config.cpp`

### Adding Content Analysis

To add new content analysis features:

1. Extend the `ContentAnalyzer` class in `content_analyzer.cpp`
2. Create new methods to extract desired features from HTML content
3. Update the `ContentFeatures` struct to include your new data
4. Modify `WebCrawler::processPage()` to use your new analysis

### Supporting New Media Types

To support additional media types:

1. Update `WebCrawler::processPage()` to detect your new media type
2. Create appropriate handler methods similar to `processImage()`
3. Extend the database schema in `database.cpp` to store the new data

## Database Schema

The crawler uses SQLite with the following core tables:

1. **pages**: Stores HTML pages and metadata
   - id, url, title, content, depth, timestamp, etc.

2. **images**: Stores information about downloaded images
   - id, url, file_path, width, height, size, etc.

3. **urls**: Stores the crawl frontier (URLs to be processed)
   - id, url, depth, source_page_id, discovered_at, etc.

4. **content_features**: Stores extracted content features
   - id, page_id, feature_name, feature_value, etc.

## Debugging and Development

### Stub Implementation

For faster development and testing, use the stub implementation:

```bash
build.bat
```

This builds the crawler with stub versions of external dependencies, making it faster to compile and test.

### Logging

Use the `Monitoring` class for logging:

```cpp
monitoring->log(Monitoring::LogLevel::INFO, "Your log message");
```

### Performance Profiling

For performance-critical sections, use the profiling methods:

```cpp
monitoring->startProfiling("operation_name");
// Your code here
monitoring->stopProfiling("operation_name");
```

## Extending Config Options

To add new configuration options:

1. Add your new option to the `Config` class in `config.hpp`
2. Add a getter method in `config.hpp`
3. Update the `parseConfig()` method in `config.cpp`
4. Update the default values in the `Config` constructor
5. Document your new option in `docs/CONFIG.md`

## Contributing

When contributing to the project:

1. Follow the existing code style
2. Write unit tests for new functionality
3. Update documentation to reflect your changes
4. Keep backward compatibility in mind
5. Optimize for performance in critical sections 