@echo off
echo Building Web Crawler with direct CMake path...

:: Check if Visual Studio is installed
where /q cl.exe
if %ERRORLEVEL% NEQ 0 (
    echo Visual Studio not found in PATH
    echo Trying to find Visual Studio...
    
    :: Try to find VS installation path
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        echo Found Visual Studio 2022 Community
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        echo Found Visual Studio 2022 Build Tools
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        echo Found Visual Studio 2019 Community
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        echo Found Visual Studio 2019 Build Tools
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        echo Found Visual Studio 2017
        call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else (
        echo ERROR: Could not find Visual Studio.
        echo Please run vs_setup.bat to install Visual Studio Build Tools.
        exit /b 1
    )
)

:: Create build directory if it doesn't exist
if not exist build mkdir build

:: Directly build with MSBuild without using CMake
echo Building directly with MSBuild...

:: Create a simple temporary makefile
echo #include "src/main.cpp" > temp_main.cpp

:: Compile it
cl /EHsc /std:c++17 /DSTUB_IMPLEMENTATION /I"include" temp_main.cpp src/crawler.cpp src/url_parser.cpp src/html_parser.cpp src/robots_parser.cpp src/sitemap_parser.cpp src/database.cpp src/scheduler.cpp src/http_client.cpp src/content_analyzer.cpp src/image_analyzer.cpp /Fe:webcrawler.exe

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo The executable is located in webcrawler.exe
) else (
    echo Build failed with error code %ERRORLEVEL%
)

exit /b %ERRORLEVEL% 