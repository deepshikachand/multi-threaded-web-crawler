@echo off
echo Building full web crawler implementation...

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

REM Check if vcpkg has nlohmann-json installed
echo Checking if nlohmann-json is installed...
if not exist "vcpkg\installed\x64-windows\include\nlohmann" (
    echo Installing nlohmann-json...
    vcpkg\vcpkg install nlohmann-json:x64-windows
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to install nlohmann-json.
        exit /b 1
    )
)

REM Check if cmake is available, if not install it
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo CMake not found, installing...
    vcpkg\vcpkg install cmake --triplet=x64-windows
    set PATH=%PATH%;%cd%\vcpkg\installed\x64-windows\tools\cmake\bin
)

REM Create data and logs directories if they don't exist
if not exist data mkdir data
if not exist data\images mkdir data\images
if not exist data\content mkdir data\content
if not exist logs mkdir logs

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Generate build files with CMake
echo Generating build files with CMake...
cd build
cmake .. -DUSE_STUB_IMPLEMENTATION=OFF -DCMAKE_TOOLCHAIN_FILE="..\vcpkg\scripts\buildsystems\vcpkg.cmake"
if %ERRORLEVEL% NEQ 0 (
    echo Failed to generate build files.
    cd ..
    exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    cd ..
    exit /b 1
)

cd ..

REM Copy config.json to build directory if it exists
if exist config.json (
    echo Copying config.json to build directory...
    copy config.json build\Release\
)

echo Build completed successfully!
echo.
echo You can run the web crawler with:
echo .\build\Release\webcrawler.exe
echo.
echo To specify a different config file:
echo .\build\Release\webcrawler.exe myconfig.json
echo.
echo For more options, run:
echo .\build\Release\webcrawler.exe --help 