# Multi-Threaded Web Crawler - Setup Guide

This guide will help you set up everything needed to build and run the web crawler project on Windows.

## Setup Steps

### 1. Install Visual Studio Build Tools

This provides the C++ compiler and necessary build tools:

1. Run the following script:
   ```
   .\vs_setup.bat
   ```

2. This will download and install Visual Studio Build Tools with C++ support.
3. After installation completes, **restart your computer**.

### 2. Install CMake

CMake is needed to generate the build files:

1. Run the following script:
   ```
   .\install_cmake.bat
   ```

2. This will download and install CMake.
3. Open a new command prompt for the changes to take effect.

### 3. Build the Stub Implementation

The stub implementation allows you to build and run the project without external dependencies:

1. Run the build script:
   ```
   .\build.bat
   ```

2. If successful, the executable will be at:
   ```
   build\Release\webcrawler.exe
   ```

3. You can run it with:
   ```
   .\build\Release\webcrawler.exe https://example.com
   ```

### 4. Install Dependencies (Optional, for Full Functionality)

If you want to build the full version with all features:

1. Run the dependency installation script:
   ```
   .\install_deps.bat
   ```

2. This will:
   - Clone vcpkg (C++ package manager)
   - Bootstrap vcpkg
   - Install basic dependencies (CURL, SQLite3, Boost)
   - Configure CMake to use vcpkg

3. Edit `CMakeLists.txt` to enable the full implementation:
   - Remove the `STUB_IMPLEMENTATION` define
   - Uncomment the dependency sections

4. Rebuild the project:
   ```
   .\build.bat
   ```

## Troubleshooting

### "Visual Studio not found"
- Make sure you ran `vs_setup.bat` and restarted your computer
- Verify installation by running `where cl.exe` in a command prompt

### "CMake not found"
- Make sure you ran `install_cmake.bat`
- Open a new command prompt and run `cmake --version`

### Build errors with dependencies
- Make sure all dependencies are installed via vcpkg
- Check that CMake is configured with the vcpkg toolchain file
- If you're having issues with specific libraries, you can:
  1. Edit `CMakeLists.txt` to disable certain features
  2. Edit header files to comment out problematic includes

## Going Further

For more advanced configurations and enabling specific features, refer to the main README.md file. 