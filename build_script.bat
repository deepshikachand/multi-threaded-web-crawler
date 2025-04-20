@echo off
echo Building Universal Web Crawler...

:: Set paths for Visual Studio - Modify these paths as needed for your system
set VS2022_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
set VS2019_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"

:: Try to find Visual Studio
if exist %VS2022_PATH% (
    echo Using Visual Studio 2022
    call %VS2022_PATH% -arch=x64
) else if exist %VS2019_PATH% (
    echo Using Visual Studio 2019
    call %VS2019_PATH% -arch=x64
) else (
    echo Visual Studio not found in standard locations.
    echo Please run this script from the Visual Studio Developer Command Prompt.
    echo Attempting to build anyway with basic compiler commands...
)

:: Create build directory if it doesn't exist
if not exist build mkdir build

:: Try to detect if CL.EXE is available
where cl.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo CL.EXE not found in path.
    echo Please run this script from a Visual Studio Developer Command Prompt.
    goto :error
)

:: Build the crawler
echo Building with CL.EXE...
cl.exe /std:c++17 /EHsc /W4 /MD /Iinclude /Febuild/universal_crawler.exe main.cpp src/universal_crawler.cpp

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    goto :error
)

:: Create data directories if they don't exist
if not exist data mkdir data
if not exist data\content mkdir data\content
if not exist data\images mkdir data\images
if not exist logs mkdir logs

echo.
echo Build successful! 
echo Run the crawler with: build\universal_crawler.exe

goto :end

:error
echo Build failed. Please check error messages above.
exit /b 1

:end
echo Build complete. 