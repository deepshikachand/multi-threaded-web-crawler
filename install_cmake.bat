@echo off
echo Downloading CMake...
curl -L -o cmake-3.30.1-windows-x86_64.msi https://github.com/Kitware/CMake/releases/download/v3.30.1/cmake-3.30.1-windows-x86_64.msi

echo Installing CMake...
msiexec /i cmake-3.30.1-windows-x86_64.msi /quiet /qn /norestart ADDLOCAL=ALL

echo Adding CMake to PATH...
setx PATH "%PATH%;C:\Program Files\CMake\bin" /M

echo CMake installation completed.
echo Please open a new command prompt for the changes to take effect. 