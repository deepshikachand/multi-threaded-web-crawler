@echo off
echo Building Hello World test...

REM Check if the Visual Studio environment is already initialized
where cl.exe >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Initializing Visual Studio environment...
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else (
        echo Visual Studio 2022 not found. Please install it first.
        exit /b 1
    )
)

echo Compiling hello.cpp...
cl.exe /EHsc /std:c++17 /Fe:hello.exe hello.cpp

if %ERRORLEVEL% neq 0 (
    echo Failed to compile hello.cpp
    exit /b 1
)

echo Build successful!
echo Run hello.exe to test 