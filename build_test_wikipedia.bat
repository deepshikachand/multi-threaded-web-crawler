@echo off
echo Building Wikipedia test crawler...

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

REM Set vcpkg paths if available
if exist vcpkg (
    set VCPKG_ROOT=%CD%\vcpkg
    set INCLUDE=%INCLUDE%;%VCPKG_ROOT%\installed\x64-windows\include
    set LIB=%LIB%;%VCPKG_ROOT%\installed\x64-windows\lib
)

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Compile with STUB_IMPLEMENTATION defined
echo Compiling Wikipedia test crawler...
cl /EHsc /std:c++17 /D STUB_IMPLEMENTATION /Fewiki_crawler.exe test_wikipedia.cpp

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful!
    echo.
    
    REM Create required directories
    if not exist data mkdir data
    if not exist data\images mkdir data\images
    if not exist data\content mkdir data\content
    if not exist logs mkdir logs
    
    echo You can run the test crawler with: wiki_crawler.exe [wikipedia_url]
    echo For example: wiki_crawler.exe https://en.wikipedia.org/wiki/Multi-threading
    echo.
    echo Press any key to run with default URL (Web crawler page)...
    pause > nul
    
    echo.
    echo Running test crawler...
    echo.
    wiki_crawler.exe
) else (
    echo Build failed!
) 