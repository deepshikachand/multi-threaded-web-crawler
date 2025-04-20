@echo off
setlocal enabledelayedexpansion

echo Building Wikipedia crawler...

REM Find Visual Studio
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    set VS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
) else (
    echo Visual Studio 2022 not found.
    exit /b 1
)

REM Initialize VS environment
call !VS_PATH! x64

REM Set vcpkg paths
set VCPKG_ROOT=%CD%\vcpkg
set INCLUDE=%INCLUDE%;%VCPKG_ROOT%\installed\x64-windows\include
set LIB=%LIB%;%VCPKG_ROOT%\installed\x64-windows\lib

REM Compile with all source files
echo Compiling Wiki Crawler...
cl /EHsc /std:c++17 /D STUB_IMPLEMENTATION /Fewiki_crawler.exe ^
    wiki_crawler_main.cpp ^
    src\config.cpp ^
    /I"%VCPKG_ROOT%\installed\x64-windows\include" ^
    /link ^
    /LIBPATH:"%VCPKG_ROOT%\installed\x64-windows\lib" ^
    libcurl.lib

if %errorlevel% equ 0 (
    echo Build successful!
    
    REM Copy DLLs needed for runtime
    echo Copying required DLLs...
    copy %VCPKG_ROOT%\installed\x64-windows\bin\libcurl.dll .
    
    REM Create required directories
    echo Creating required directories...
    if not exist data mkdir data
    if not exist data\images mkdir data\images
    if not exist data\content mkdir data\content
    if not exist logs mkdir logs
    
    echo.
    echo You can run the crawler with: wiki_crawler.exe [config_file]
    echo The default config file is config.json
    echo.
    echo Press any key to run with default settings...
    pause > nul
    
    wiki_crawler.exe
) else (
    echo Build failed.
)

endlocal 