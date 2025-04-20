@echo off
setlocal enabledelayedexpansion

echo ===================================
echo Building Web Crawler
echo ===================================
echo.

REM Find Visual Studio installation
set VS_PATH=
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
    set VS_PATH=%%i
)

if "%VS_PATH%"=="" (
    echo Could not find Visual Studio installation.
    exit /b 1
)

if exist "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" (
    echo Found Visual Studio 2022 Community
    call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo Could not find Visual Studio build tools.
    exit /b 1
)

REM Ask which version to build
set /p BUILD_VERSION="Which version do you want to build? (1=Simple, 2=Full): "

if "%BUILD_VERSION%"=="1" (
    echo Building simple version...
    set BUILD_FLAGS=/DMINIMAL_BUILD=1 /DUSE_STUB_IMPLEMENTATION=1
) else (
    echo Building full version...
    set BUILD_FLAGS=
)

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Compile with C++17 standard
set COMPILER_FLAGS=/std:c++17 /EHsc /W4 /Zi /MD %BUILD_FLAGS%
set INCLUDE_DIRS=/I"include"

REM Compile source files
for %%f in (
    src\main.cpp
    src\url_parser.cpp
    src\crawler.cpp
    src\thread_pool.cpp
    src\database.cpp
    src\file_indexer.cpp
    src\image_analyzer.cpp
    src\monitoring.cpp
    src\resource_manager.cpp
    src\content_analyzer.cpp
    src\config.cpp
    src\universal_crawler.cpp
) do (
    echo Compiling %%~nf.cpp...
    cl.exe %COMPILER_FLAGS% %INCLUDE_DIRS% /c %%f /Fo"build\%%~nf.obj"
    if errorlevel 1 (
        echo Error compiling %%~nf.cpp
        exit /b 1
    )
)

REM Link object files
echo Linking...
cl.exe %COMPILER_FLAGS% /Fe"build\webcrawler.exe" build\*.obj
if errorlevel 1 (
    echo Error linking
    exit /b 1
)

echo.
echo Build successful!
echo Executable is in build\webcrawler.exe

REM Ask what action to take
echo.
echo ===================================
echo What would you like to do next?
echo ===================================
echo 1. Run the crawler
echo 2. View database statistics only
echo 3. Exit
echo.

set /p ACTION="Enter your choice (1-3): "

if "!ACTION!"=="1" (
    echo.
    echo ===================================
    echo Web Crawler Configuration
    echo ===================================
    
    REM Get crawling parameters
    set /p SEED_URL="Enter starting URL (e.g., https://example.com): "
    set /p MAX_THREADS="Enter number of threads (default: 4): "
    set /p MAX_DEPTH="Enter maximum crawl depth (default: 3): "
    set /p ALLOWED_DOMAINS="Enter allowed domains (comma separated, press Enter to use domain from URL): "
    
    REM Set defaults if empty
    if "!MAX_THREADS!"=="" set MAX_THREADS=4
    if "!MAX_DEPTH!"=="" set MAX_DEPTH=3
    
    REM If allowed domains is empty, extract domain from seed URL
    if "!ALLOWED_DOMAINS!"=="" (
        for /f "tokens=2 delims=://" %%a in ("!SEED_URL!") do (
            set DOMAIN=%%a
            for /f "tokens=1 delims=/" %%b in ("!DOMAIN!") do (
                set ALLOWED_DOMAINS=%%b
            )
        )
        echo Domain extracted from URL: !ALLOWED_DOMAINS!
    )
    
    echo.
    echo Starting crawler with:
    echo - URL: !SEED_URL!
    echo - Threads: !MAX_THREADS!
    echo - Max Depth: !MAX_DEPTH!
    echo - Allowed Domains: !ALLOWED_DOMAINS!
    echo.
    
    REM Run the crawler with parameters
    build\webcrawler.exe --url "!SEED_URL!" --threads !MAX_THREADS! --depth !MAX_DEPTH! --allowed-domains "!ALLOWED_DOMAINS!"
) else if "!ACTION!"=="2" (
    echo.
    echo ===================================
    echo Database Statistics
    echo ===================================
    echo Viewing previously crawled data...
    
    REM Run webcrawler with a special stats-only flag
    build\webcrawler.exe --stats-only
) else (
    echo Exiting...
)

echo.
echo Done!
endlocal 