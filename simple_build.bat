@echo off
echo Building minimal web crawler for testing...

REM Try to find VS installation path
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Found Visual Studio 2022 Community
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Found Visual Studio 2022 Build Tools
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
) else (
    echo Could not find Visual Studio
    exit /b 1
)

REM Create a simple main.cpp file for testing
echo #include ^<iostream^> > simple_main.cpp
echo #include "include/crawler.hpp" >> simple_main.cpp
echo int main(int argc, char* argv[]) { >> simple_main.cpp
echo     std::cout ^<^< "Web Crawler - Stub Implementation" ^<^< std::endl; >> simple_main.cpp
echo     if (argc ^< 2) { >> simple_main.cpp
echo         std::cout ^<^< "Usage: " ^<^< argv[0] ^<^< " ^<url^> [threads] [depth]" ^<^< std::endl; >> simple_main.cpp
echo         return 1; >> simple_main.cpp
echo     } >> simple_main.cpp
echo     std::string url = argv[1]; >> simple_main.cpp
echo     int threads = (argc ^> 2) ? std::stoi(argv[2]) : 4; >> simple_main.cpp
echo     int depth = (argc ^> 3) ? std::stoi(argv[3]) : 3; >> simple_main.cpp
echo     std::cout ^<^< "Would crawl: " ^<^< url ^<^< std::endl; >> simple_main.cpp
echo     std::cout ^<^< "Threads: " ^<^< threads ^<^< std::endl; >> simple_main.cpp
echo     std::cout ^<^< "Depth: " ^<^< depth ^<^< std::endl; >> simple_main.cpp
echo     return 0; >> simple_main.cpp
echo } >> simple_main.cpp

REM Compile with STUB_IMPLEMENTATION defined
cl /EHsc /std:c++17 /D STUB_IMPLEMENTATION simple_main.cpp /Fesimple_crawler.exe

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Run the crawler with: simple_crawler.exe https://example.com
) else (
    echo Build failed!
) 