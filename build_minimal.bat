@echo off
setlocal enabledelayedexpansion

echo Building minimal crawler...

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set VS_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    set VS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
) else (
    echo Visual Studio 2022 not found.
    exit /b 1
)

echo Using Visual Studio at: !VS_PATH!

echo Initializing Visual Studio environment...
call !VS_PATH! x64

echo Setting up vcpkg paths...
set VCPKG_ROOT=%CD%\vcpkg
set INCLUDE=%INCLUDE%;%VCPKG_ROOT%\installed\x64-windows\include
set LIB=%LIB%;%VCPKG_ROOT%\installed\x64-windows\lib

echo Compiling minimal crawler...
cl /EHsc /std:c++17 /Fe:minimal_crawler.exe minimal_crawler.cpp /I"%VCPKG_ROOT%\installed\x64-windows\include" /link libcurl.lib /LIBPATH:"%VCPKG_ROOT%\installed\x64-windows\lib"

if %errorlevel% equ 0 (
    echo Build successful!
    
    echo Copying required DLLs...
    copy %VCPKG_ROOT%\installed\x64-windows\bin\libcurl.dll .
    
    echo You can run with: minimal_crawler.exe [url] [depth]
) else (
    echo Build failed.
)

endlocal 