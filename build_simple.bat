@echo off
echo Building simple web crawler...

REM Check if dependencies are installed, if not install them
if not exist vcpkg (
    echo Dependencies not found, installing...
    call install_deps.bat
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to install dependencies.
        exit /b 1
    )
)

REM Initialize Visual Studio environment
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

REM Get VCPKG include and lib paths
set VCPKG_INCLUDE_DIR=%cd%\vcpkg\installed\x64-windows\include
set VCPKG_LIB_DIR=%cd%\vcpkg\installed\x64-windows\lib
set VCPKG_DLL_DIR=%cd%\vcpkg\installed\x64-windows\bin

echo Building basic_crawler.cpp...
cl /EHsc /std:c++17 /I "%VCPKG_INCLUDE_DIR%" basic_crawler.cpp /Fe:web_crawler.exe "%VCPKG_LIB_DIR%\libcurl.lib" "%VCPKG_LIB_DIR%\sqlite3.lib"

if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b 1
)

REM Copy DLLs to current directory
copy "%VCPKG_DLL_DIR%\libcurl.dll" .
copy "%VCPKG_DLL_DIR%\sqlite3.dll" .
copy "%VCPKG_DLL_DIR%\zlib1.dll" .

echo Build completed successfully!
echo.
echo You can run the web crawler with:
echo web_crawler.exe https://example.com 