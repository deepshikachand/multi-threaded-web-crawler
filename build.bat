@echo off
setlocal enabledelayedexpansion

echo Building Multi-threaded Web Crawler...

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

:: Check for vcpkg
if not exist "vcpkg" (
    echo vcpkg not found. Please run install_dependencies.bat first.
    exit /b 1
)

:: Initialize Visual Studio environment
echo Initializing Visual Studio environment...
call %VS_PATH% x64

:: Set vcpkg paths
set VCPKG_ROOT=%CD%\vcpkg
set INCLUDE=%INCLUDE%;%VCPKG_ROOT%\installed\x64-windows\include
set LIB=%LIB%;%VCPKG_ROOT%\installed\x64-windows\lib

:: Create build directory
if not exist build mkdir build

echo Compiling Multi-threaded Web Crawler...

:: Compile all source files
cl.exe /std:c++17 /EHsc /Fo"build\\" /Fe"build\web_crawler.exe" ^
    /I"include" ^
    /I"%VCPKG_ROOT%\installed\x64-windows\include" ^
    src\*.cpp ^
    /link ^
    /LIBPATH:"%VCPKG_ROOT%\installed\x64-windows\lib" ^
    libcurl.lib sqlite3.lib ws2_32.lib

if %errorlevel% neq 0 (
    echo Build failed.
    exit /b 1
)

:: Copy DLLs to build directory
echo Copying required DLLs...
copy %VCPKG_ROOT%\installed\x64-windows\bin\libcurl.dll build\
copy %VCPKG_ROOT%\installed\x64-windows\bin\sqlite3.dll build\

:: Create data and logs directories
echo Creating required directories...
if not exist data mkdir data
if not exist data\images mkdir data\images
if not exist data\content mkdir data\content
if not exist logs mkdir logs

:: Copy config file to build directory
echo Copying config file...
copy config.json build\

echo Build completed successfully!
echo You can run the web crawler using: build\web_crawler.exe 