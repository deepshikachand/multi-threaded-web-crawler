@echo off
setlocal enabledelayedexpansion

echo Building Web Crawler Test...

:: Check for Visual Studio environment
set VS_PATH=""
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
)

if %VS_PATH% == "" (
    echo Visual Studio 2022 not found. Please run install_dependencies.bat first.
    exit /b 1
)

:: Initialize Visual Studio environment
echo Initializing Visual Studio environment...
call %VS_PATH% x64

:: Set vcpkg paths
set VCPKG_ROOT=%CD%\vcpkg
set INCLUDE=%INCLUDE%;%VCPKG_ROOT%\installed\x64-windows\include
set LIB=%LIB%;%VCPKG_ROOT%\installed\x64-windows\lib

:: Create test directory
if not exist test mkdir test

echo Compiling Web Crawler Test...

:: Compile the test program
cl.exe /std:c++17 /EHsc /Fo"test\\" /Fe"test\crawler_test.exe" ^
    /I"include" ^
    /I"%VCPKG_ROOT%\installed\x64-windows\include" ^
    test_crawler.cpp src\config.cpp ^
    /link ^
    /LIBPATH:"%VCPKG_ROOT%\installed\x64-windows\lib" ^
    ws2_32.lib

if %errorlevel% neq 0 (
    echo Test build failed.
    exit /b 1
)

:: Copy DLLs to test directory
echo Copying required DLLs...
copy %VCPKG_ROOT%\installed\x64-windows\bin\*.dll test\

:: Copy configuration file
if exist config.json (
    echo Copying config.json to test directory...
    copy config.json test\
) else (
    echo Creating sample config.json...
    echo { > test\config.json
    echo     "crawler": { >> test\config.json
    echo         "start_url": "https://example.com", >> test\config.json
    echo         "max_depth": 3, >> test\config.json
    echo         "max_pages": 1000, >> test\config.json
    echo         "user_agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Crawler-Test/1.0", >> test\config.json
    echo         "respect_robots_txt": true, >> test\config.json
    echo         "follow_redirects": true, >> test\config.json
    echo         "timeout_seconds": 30, >> test\config.json
    echo         "retry_count": 3 >> test\config.json
    echo     }, >> test\config.json
    echo     "threading": { >> test\config.json
    echo         "thread_count": 4, >> test\config.json
    echo         "queue_size_limit": 1000, >> test\config.json
    echo         "batch_size": 10 >> test\config.json
    echo     }, >> test\config.json
    echo     "storage": { >> test\config.json
    echo         "database_path": "data/crawler.db", >> test\config.json
    echo         "save_html": true, >> test\config.json
    echo         "save_images": true, >> test\config.json
    echo         "image_directory": "data/images", >> test\config.json
    echo         "content_directory": "data/content" >> test\config.json
    echo     }, >> test\config.json
    echo     "filters": { >> test\config.json
    echo         "allowed_domains": ["example.com"], >> test\config.json
    echo         "excluded_paths": ["/admin", "/login"], >> test\config.json
    echo         "allowed_extensions": [".html", ".htm"], >> test\config.json
    echo         "image_extensions": [".jpg", ".png", ".gif"] >> test\config.json
    echo     }, >> test\config.json
    echo     "monitoring": { >> test\config.json
    echo         "log_level": "INFO", >> test\config.json
    echo         "log_file": "logs/crawler.log", >> test\config.json
    echo         "enable_console_output": true, >> test\config.json
    echo         "status_update_interval": 5 >> test\config.json
    echo     } >> test\config.json
    echo } >> test\config.json
)

echo Test build completed successfully!
echo.
echo You can run the test with:
echo test\crawler_test.exe 