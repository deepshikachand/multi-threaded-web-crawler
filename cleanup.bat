@echo off
echo ===================================
echo Cleaning up Web Crawler Project
echo ===================================
echo.

echo This script will remove temporary files and clean up the project.
echo The following items will be removed:
echo - Temporary build files (*.obj, *.pdb, etc.)
echo - Temporary and backup executables
echo - Unnecessary batch files
echo.

set /p CONFIRM=Do you want to proceed? (Y/N): 
if /i not "%CONFIRM%"=="Y" goto end

echo.
echo Removing object files...
del /s /q *.obj 2>nul
del /s /q *.pdb 2>nul
del /s /q *.ilk 2>nul
del /s /q *.exp 2>nul
del /s /q *.lib 2>nul

echo Removing unnecessary batch files...
del /q build_hello.bat 2>nul
del /q build_minimal.bat 2>nul
del /q build_simple.bat 2>nul
del /q build_test.bat 2>nul
del /q build_test_wikipedia.bat 2>nul
del /q build_wiki_crawler.bat 2>nul
del /q temp_build.bat 2>nul

echo Removing unnecessary/backup executables...
del /q hello.exe 2>nul
del /q temp_main.exe 2>nul
del /q minimal_crawler_backup.exe 2>nul

echo Preserving essential files:
echo - build_crawler.bat (main build script)
echo - run_crawler.bat (main run script)
echo - web_crawler.exe or simple_crawler.exe (if they exist)
echo - crawler_config.json (configuration file)

echo.
echo Cleanup complete!

:end
echo.
pause 