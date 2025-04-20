@echo off
setlocal enabledelayedexpansion

echo Building Universal Web Crawler...

:: Set environment
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
) else (
    echo Visual Studio not found. Please install Visual Studio with C++ development tools.
    exit /b 1
)

:: Create build directory if it doesn't exist
if not exist build mkdir build

:: Build the crawler
cl.exe /std:c++17 /EHsc /W4 /MD /I"include" /Febuild/universal_crawler.exe universal_crawler.cpp /link /LIBPATH:"lib" libcurl.lib

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

:: Create data directories if they don't exist
if not exist data mkdir data
if not exist data\content mkdir data\content
if not exist data\images mkdir data\images
if not exist logs mkdir logs

echo.
echo Build successful! 
echo Run the crawler with: universal_crawler.exe [URL] [max_depth] [allowed_domains]
echo Default values: example.com, depth 2, domains: example.com,www.example.com
echo.
echo Example: universal_crawler.exe https://example.com 2 example.com,www.example.com

endlocal 