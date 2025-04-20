@echo off
setlocal enabledelayedexpansion

echo ===================================
echo Multi-Threaded Web Crawler
echo ===================================
echo.

REM Check if crawler exists, if not build it
if not exist web_crawler.exe (
    if not exist simple_crawler.exe (
        echo No crawler executable found. Building first...
        call build_crawler.bat
        if not exist web_crawler.exe (
            if not exist simple_crawler.exe (
                echo Failed to build crawler.
                goto error
            )
        )
    )
)

REM Get user input for crawl parameters
set /p WEBSITE=Enter the website URL to crawl (e.g., https://example.com): 
set /p THREADS=Enter the number of threads (1-16, default: 4): 
set /p DEPTH=Enter the max crawl depth (1-10, default: 2): 
set /p MAX_PAGES=Enter the max pages to crawl (10-1000, default: 50): 

REM Set default values if empty
if "!THREADS!"=="" set THREADS=4
if "!DEPTH!"=="" set DEPTH=2
if "!MAX_PAGES!"=="" set MAX_PAGES=50

REM Validate URL
echo !WEBSITE! | findstr /i "^https://" > nul
if errorlevel 1 (
    echo !WEBSITE! | findstr /i "^http://" > nul
    if errorlevel 1 (
        echo URL must start with http:// or https://
        goto error
    )
)

REM Get domain from URL
for /f "tokens=*" %%a in ('powershell -command "(New-Object System.Uri('!WEBSITE!')).Host"') do (
    set DOMAIN=%%a
)

echo.
echo Crawler will use these settings:
echo - Website: !WEBSITE!
echo - Domain: !DOMAIN!
echo - Threads: !THREADS!
echo - Max depth: !DEPTH!
echo - Max pages: !MAX_PAGES!
echo.

REM Ask for confirmation
set /p CONFIRM=Proceed with these settings? (Y/N): 
if /i not "!CONFIRM!"=="Y" goto end

REM Create config file
echo Creating configuration...
(
echo {
echo     "crawler": {
echo         "start_url": "!WEBSITE!",
echo         "max_depth": !DEPTH!,
echo         "max_pages": !MAX_PAGES!,
echo         "user_agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Multi-Threaded-Web-Crawler/1.0",
echo         "respect_robots_txt": true,
echo         "follow_redirects": true,
echo         "timeout_seconds": 30,
echo         "retry_count": 3
echo     },
echo     "threading": {
echo         "thread_count": !THREADS!,
echo         "queue_size_limit": 1000,
echo         "batch_size": 5
echo     },
echo     "storage": {
echo         "database_path": "data/crawler.db",
echo         "save_html": true,
echo         "save_images": true,
echo         "image_directory": "data/images",
echo         "content_directory": "data/content"
echo     },
echo     "filters": {
echo         "allowed_domains": ["!DOMAIN!"],
echo         "excluded_paths": [
echo             "/login",
echo             "/logout",
echo             "/admin",
echo             "/cart",
echo             "/checkout",
echo             "/account",
echo             "/signup",
echo             "/register"
echo         ],
echo         "allowed_extensions": [".html", ".htm", ".php", ".asp", ".aspx", ".jsp"],
echo         "image_extensions": [".jpg", ".jpeg", ".png", ".gif", ".webp", ".svg"]
echo     },
echo     "monitoring": {
echo         "log_level": "INFO",
echo         "log_file": "logs/crawler.log",
echo         "enable_console_output": true,
echo         "status_update_interval_seconds": 5
echo     },
echo     "advanced": {
echo         "request_delay_ms": 200,
echo         "max_file_size_mb": 10,
echo         "detect_language": true,
echo         "extract_metadata": true,
echo         "handle_javascript": false,
echo         "respect_canonical": true,
echo         "max_redirect_count": 5,
echo         "detect_duplicates": true,
echo         "handle_cookies": true
echo     }
echo }
) > crawler_config.json

REM Create necessary directories
if not exist data mkdir data
if not exist data\images mkdir data\images
if not exist data\content mkdir data\content
if not exist logs mkdir logs

REM Run the appropriate crawler executable
if exist web_crawler.exe (
    echo Running with web_crawler.exe...
    web_crawler.exe crawler_config.json
) else if exist simple_crawler.exe (
    echo Running with simple_crawler.exe...
    simple_crawler.exe crawler_config.json
) else if exist wiki_crawler.exe (
    echo Running with wiki_crawler.exe...
    wiki_crawler.exe crawler_config.json
) else (
    echo No crawler executable found.
    goto error
)

goto end

:error
echo.
echo An error occurred during setup.
exit /b 1

:end
echo.
echo Crawler finished. Results are stored in the data directory.
pause
exit /b 0 