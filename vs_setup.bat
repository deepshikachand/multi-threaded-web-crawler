@echo off
echo Downloading Visual Studio Build Tools...
curl -L -o vs_buildtools.exe https://aka.ms/vs/17/release/vs_buildtools.exe

echo Installing Visual Studio Build Tools...
vs_buildtools.exe --quiet --wait --norestart --nocache ^
    --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools" ^
    --add Microsoft.VisualStudio.Workload.VCTools ^
    --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
    --add Microsoft.VisualStudio.Component.Windows10SDK.19041

echo Installation initiated. This may take a while...
echo When installation completes, please restart your computer. 