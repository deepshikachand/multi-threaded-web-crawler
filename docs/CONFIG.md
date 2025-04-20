# Configuration Guide

The Multi-Threaded Web Crawler uses a JSON configuration file to control its behavior. This document explains all available configuration options.

## Basic Usage

By default, the crawler looks for a file named `config.json` in the same directory as the executable. You can specify a different configuration file by passing it as a command-line argument:

```
.\build\Release\webcrawler.exe custom_config.json
```

You can also override specific settings via command-line arguments, which take precedence over the configuration file:

```
.\build\Release\webcrawler.exe --url https://example.com --threads 4 --depth 3
```

## Configuration File Format

The configuration file uses JSON format with the following structure:

```json
{
    "crawler": {
        "start_url": "https://example.com",
        "max_depth": 3,
        "max_pages": 1000,
        "user_agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Multi-Threaded-Web-Crawler/1.0",
        "respect_robots_txt": true,
        "follow_redirects": true,
        "timeout_seconds": 30,
        "retry_count": 3
    },
    "threading": {
        "thread_count": 8,
        "queue_size_limit": 10000,
        "batch_size": 20
    },
    "storage": {
        "database_path": "data/crawler.db",
        "save_html": true,
        "save_images": true,
        "image_directory": "data/images",
        "content_directory": "data/content"
    },
    "filters": {
        "allowed_domains": ["example.com", "sub.example.com"],
        "excluded_paths": ["/admin", "/login", "/private"],
        "allowed_extensions": [".html", ".php", ".htm", ".asp", ".aspx"],
        "image_extensions": [".jpg", ".jpeg", ".png", ".gif", ".webp", ".svg"]
    },
    "monitoring": {
        "log_level": "INFO",
        "log_file": "logs/crawler.log",
        "enable_console_output": true,
        "status_update_interval": 10
    }
}
```

## Configuration Options

### Crawler Settings

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `start_url` | string | - | The initial URL to start crawling from |
| `max_depth` | integer | 3 | Maximum depth of links to follow from the start URL |
| `max_pages` | integer | 1000 | Maximum number of pages to crawl before stopping |
| `user_agent` | string | "Mozilla/5.0..." | User agent string to use in HTTP requests |
| `respect_robots_txt` | boolean | true | Whether to obey robots.txt rules |
| `follow_redirects` | boolean | true | Whether to follow HTTP redirects |
| `timeout_seconds` | integer | 30 | Request timeout in seconds |
| `retry_count` | integer | 3 | Number of retry attempts for failed requests |

### Threading Settings

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `thread_count` | integer | 8 | Number of worker threads to use for crawling |
| `queue_size_limit` | integer | 10000 | Maximum size of the URL queue before throttling |
| `batch_size` | integer | 20 | Number of URLs to process in a batch |

### Storage Settings

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `database_path` | string | "data/crawler.db" | Path to the SQLite database file |
| `save_html` | boolean | true | Whether to save full HTML content |
| `save_images` | boolean | true | Whether to download and save images |
| `image_directory` | string | "data/images" | Directory to store downloaded images |
| `content_directory` | string | "data/content" | Directory to store crawled content |

### Filter Settings

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `allowed_domains` | array | [] | List of domains to crawl (empty = only domain from start_url) |
| `excluded_paths` | array | [] | URL paths to exclude from crawling |
| `allowed_extensions` | array | [".html", ...] | File extensions to crawl |
| `image_extensions` | array | [".jpg", ...] | File extensions to treat as images |

### Monitoring Settings

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `log_level` | string | "INFO" | Log level (DEBUG, INFO, WARNING, ERROR) |
| `log_file` | string | "logs/crawler.log" | Path to log file |
| `enable_console_output` | boolean | true | Whether to show logs in console |
| `status_update_interval` | integer | 10 | Interval in seconds between status updates |

## Advanced Configuration

### Rate Limiting

The crawler automatically implements a rate limiting mechanism to prevent overloading target servers. The `thread_count` setting indirectly controls this rate - fewer threads mean fewer simultaneous connections.

### Robots.txt Compliance

When `respect_robots_txt` is enabled, the crawler will:
1. Fetch and parse the robots.txt file for each domain
2. Respect any `Disallow` directives for the configured user agent
3. Honor the `Crawl-delay` directive if specified

### Storage Considerations

For large crawls, be aware of these storage settings:
- Set `save_html` to `false` to avoid storing full page content
- Set `save_images` to `false` to avoid downloading images
- Monitor the database size as it will grow with each crawled page 