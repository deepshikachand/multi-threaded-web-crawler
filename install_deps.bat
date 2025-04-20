@echo off
echo Installing dependencies...

REM Check if Visual Studio Build Tools are installed
if not exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        echo Visual Studio Build Tools not found.
        echo Running VS Build Tools installer...
        if exist vs_buildtools.exe (
            start /wait vs_buildtools.exe --quiet --wait --norestart --nocache ^
                --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools" ^
                --add Microsoft.VisualStudio.Workload.VCTools ^
                --includeRecommended
        ) else (
            echo vs_buildtools.exe not found.
            echo Please run vs_setup.bat first or manually install Visual Studio Build Tools.
            exit /b 1
        )
    )
)

REM Clone and install vcpkg if not already installed
if not exist vcpkg (
    echo Cloning vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    call bootstrap-vcpkg.bat
    cd ..
)

REM Install required libraries
echo Installing required libraries using vcpkg...
vcpkg\vcpkg install curl:x64-windows
vcpkg\vcpkg install sqlite3:x64-windows
vcpkg\vcpkg install nlohmann-json:x64-windows

REM Create required directories
echo Creating required directories...
if not exist data mkdir data
if not exist data\images mkdir data\images
if not exist data\content mkdir data\content
if not exist logs mkdir logs

echo Dependencies installation completed! 