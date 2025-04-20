@echo off
setlocal enabledelayedexpansion

echo Setting up dependencies for Web Crawler...

:: Check for Visual Studio
set VS_FOUND=0
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Found Visual Studio 2022 Community...
    set VS_FOUND=1
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Found Visual Studio 2022 Build Tools...
    set VS_FOUND=1
)

if %VS_FOUND% == 0 (
    echo Visual Studio 2022 not found.
    echo Downloading Visual Studio Build Tools...
    
    if not exist vs_buildtools.exe (
        curl -L -o vs_buildtools.exe https://aka.ms/vs/17/release/vs_buildtools.exe
    )
    
    echo Installing Visual Studio Build Tools with C++ components...
    start /wait vs_buildtools.exe --quiet --wait --norestart --nocache ^
        --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools" ^
        --add Microsoft.VisualStudio.Workload.VCTools ^
        --includeRecommended
    
    if %errorlevel% neq 0 (
        echo Failed to install Visual Studio Build Tools.
        echo Please install Visual Studio 2022 or Build Tools manually from https://visualstudio.microsoft.com/
        exit /b 1
    )
)

:: Set up vcpkg if not already installed
if not exist vcpkg (
    echo Setting up vcpkg...
    
    :: Clone vcpkg repository
    git clone https://github.com/Microsoft/vcpkg.git
    if %errorlevel% neq 0 (
        echo Failed to clone vcpkg. Please make sure git is installed.
        exit /b 1
    )
    
    :: Build vcpkg
    cd vcpkg
    call bootstrap-vcpkg.bat
    if %errorlevel% neq 0 (
        echo Failed to bootstrap vcpkg.
        cd ..
        exit /b 1
    )
    cd ..
) else (
    echo vcpkg already installed, updating...
    cd vcpkg
    git pull
    cd ..
)

:: Install required packages
echo Installing required libraries...
cd vcpkg
vcpkg install curl:x64-windows sqlite3:x64-windows nlohmann-json:x64-windows
if %errorlevel% neq 0 (
    echo Failed to install packages.
    cd ..
    exit /b 1
)
cd ..

:: Create required directories
echo Creating required directories...
if not exist data mkdir data
if not exist data\images mkdir data\images
if not exist data\content mkdir data\content
if not exist logs mkdir logs

echo All dependencies installed successfully!
echo Run build.bat to compile the web crawler. 