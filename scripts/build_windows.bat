@echo off
setlocal

:: ============================================================================
:: Windows Build Script (Batch)
:: ============================================================================

:: ----------------------------------------------------------------------------
:: Default Configuration
:: ----------------------------------------------------------------------------
set "BUILD_TYPE=Debug"
set "DO_CLEAN=false"
set "INCLUDE_SERVER=true"
set "INCLUDE_GUI=true"
set "ARCH=x86_64"

:: Get project root directory (parent of the script's 'deploy' directory)
for %%A in ("%~dp0..") do set "PROJECT_ROOT=%%~fA"
set "BUILD_DIR=%PROJECT_ROOT%\server\build\windows-%ARCH%"
set "OUTPUT_DIR=%PROJECT_ROOT%\dist"

:: ----------------------------------------------------------------------------
:: Argument Parsing
:: ----------------------------------------------------------------------------
:ParseArgs
if "%~1"=="" goto EndParseArgs
if /i "%~1"=="--release" set "BUILD_TYPE=Release"
if /i "%~1"=="--debug" set "BUILD_TYPE=Debug"
if /i "%~1"=="--clean" set "DO_CLEAN=true"
if /i "%~1"=="--no-server" set "INCLUDE_SERVER=false"
if /i "%~1"=="--no-gui" set "INCLUDE_GUI=false"
if /i "%~1"=="--arch" (
    set "ARCH=%~2"
    shift
)
if /i "%~1"=="--msys2" (
    set "MSYS2_PATH_ARG=%~2"
    shift
)
if /i "%~1"=="-h" goto ShowHelp
if /i "%~1"=="--help" goto ShowHelp
shift
goto ParseArgs
:EndParseArgs

:: Validate architecture
if /i not "%ARCH%"=="x86_64" if /i not "%ARCH%"=="arm64" (
    echo [ERROR] Invalid architecture specified: %ARCH%.
    echo [HINT]  Supported architectures are: x86_64, arm64.
    exit /b 1
)


:: ----------------------------------------------------------------------------
:: Find MSYS2 and Set Environment
:: ----------------------------------------------------------------------------
echo. & echo [INFO] Finding MSYS2 installation...

:: Prioritize command-line argument
if defined MSYS2_PATH_ARG (
    set "MSYS2_ROOT=%MSYS2_PATH_ARG%"
) else (
    :: Check common paths if no argument is provided (most specific first)
    if exist "D:\dev\msys64" set "MSYS2_ROOT=D:\dev\msys64"
    if not defined MSYS2_ROOT if exist "D:\msys64" set "MSYS2_ROOT=D:\msys64"
    if not defined MSYS2_ROOT if exist "C:\msys64" set "MSYS2_ROOT=C:\msys64"
)

if not defined MSYS2_ROOT (
    echo [ERROR] MSYS2 root directory not found.
    echo [HINT]  Please install MSYS2 or specify the path using the --msys2 option.
    exit /b 1
)

:: --- Path Normalization and Environment Setup ---
:: Normalize all paths to use forward slashes ('/') for maximum compatibility with build tools.
set "MSYS2_ROOT=%MSYS2_ROOT:\=/%"
if /i "%ARCH%"=="arm64" (
    set "MSYS2_ENV_ROOT=%MSYS2_ROOT%/clangarm64"
) else (
    set "MSYS2_ENV_ROOT=%MSYS2_ROOT%/ucrt64"
)

:: Set MSYS2_ROOT as an environment variable for child processes (like CMake) to inherit.
:: The 'setlocal' at the top ensures this variable is temporary and only for this script's execution.
set "MSYS2_ROOT=%MSYS2_ROOT%"

:: Add the toolchain's bin directory to the PATH to ensure tools like gcc, ninja, etc., are found.
set "PATH=%MSYS2_ENV_ROOT%/bin;%PATH%"

set "CMAKE_BIN=%MSYS2_ENV_ROOT%/bin/cmake.exe"

echo [INFO] Using MSYS2 at: %MSYS2_ROOT%


:: ----------------------------------------------------------------------------
:: Main Build Logic
:: ----------------------------------------------------------------------------



echo.
echo [INFO] Starting Windows build...
echo [INFO] Build Type: %BUILD_TYPE%

if "%DO_CLEAN%"=="true" (
    echo.
    echo [INFO] Cleaning previous build artifacts...
    if exist "%OUTPUT_DIR%" (
        echo [CLEAN] Removing %OUTPUT_DIR%
        rd /s /q "%OUTPUT_DIR%"
    )
    if exist "%PROJECT_ROOT%\server\build" (
        echo [CLEAN] Removing %PROJECT_ROOT%\server\build
        rd /s /q "%PROJECT_ROOT%\server\build"
    )
)

if "%INCLUDE_SERVER%"=="true" (
    call :BuildServer
    if errorlevel 1 (
        echo.
        echo [FATAL] Server build failed. Aborting.
        exit /b 1
    )
)

if "%INCLUDE_GUI%"=="true" (
    call :BuildGui
    if errorlevel 1 (
        echo.
        echo [FATAL] GUI build failed. Aborting.
        exit /b 1
    )
)

echo.
echo [SUCCESS] Build finished successfully.
goto :eof


:: ============================================================================
:: Build Functions (Subroutines)
:: ============================================================================

:BuildServer
    echo.
    echo ----------------------------------------
    echo [BUILD] Building Server...
    echo ----------------------------------------

    set "SERVER_BUILD_DIR=%BUILD_DIR%"
    mkdir "%SERVER_BUILD_DIR%"
    pushd "%SERVER_BUILD_DIR%"

    echo.
    echo [SERVER] Configuring CMake...
    if /i "%ARCH%"=="arm64" (
        set "TOOLCHAIN_PATH_NATIVE=%PROJECT_ROOT%\server\toolchains\windows-on-windows-arm64.cmake"
    ) else (
        set "TOOLCHAIN_PATH_NATIVE=%PROJECT_ROOT%\server\toolchains\windows-on-windows-x86.cmake"
    )
    set "TOOLCHAIN_PATH_CMAKE=%TOOLCHAIN_PATH_NATIVE:\=/%"
    "%CMAKE_BIN%" ../.. -G "Ninja" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_PATH_CMAKE%"
    if errorlevel 1 popd & exit /b 1

    echo.
    echo [SERVER] Building project with CMake...
    "%CMAKE_BIN%" --build . --config %BUILD_TYPE%
    if errorlevel 1 popd & exit /b 1

    echo.
    echo [SERVER] Copying artifacts...
    set "BINARY_NAME=server.exe"
    set "SOURCE_PATH=%SERVER_BUILD_DIR%\bin\%BUILD_TYPE%\%BINARY_NAME%"
    set "CONFIG_PATH=%PROJECT_ROOT%\server\server.conf"
    set "DEST_PATH=%OUTPUT_DIR%\windows-%ARCH%"

    :: Fallback for cases where build type is not in path
    if not exist "%SOURCE_PATH%" set "SOURCE_PATH=%SERVER_BUILD_DIR%\bin\%BINARY_NAME%"

    if not exist "%SOURCE_PATH%" (
        echo [ERROR] Build artifact not found at %SOURCE_PATH%
        popd & exit /b 1
    )

    mkdir "%DEST_PATH%" > nul 2>&1
    copy /Y "%SOURCE_PATH%" "%DEST_PATH%\%BINARY_NAME%" > nul
    if errorlevel 1 (
        echo [ERROR] Failed to copy %BINARY_NAME%.
        popd & exit /b 1
    )
    copy /Y "%CONFIG_PATH%" "%DEST_PATH%\server.conf" > nul
    if errorlevel 1 (
        echo [ERROR] Failed to copy server.conf.
        popd & exit /b 1
    )

    rem Package Windows-specific server scripts only
    if exist "%PROJECT_ROOT%\server\scripts\windows" (
        echo [SERVER] Packaging Windows scripts...
        mkdir "%DEST_PATH%\scripts" > nul 2>&1
        xcopy "%PROJECT_ROOT%\server\scripts\windows" "%DEST_PATH%\scripts" /E /I /Y /Q > nul
    )

    echo [SERVER] Server build successful.
    popd
    goto :eof

:BuildGui
    echo.
    echo ----------------------------------------
    echo [BUILD] Building GUI...
    echo ----------------------------------------

    pushd "%PROJECT_ROOT%\server_gui"

    echo.
    echo [GUI] Checking for Flutter...
    where flutter >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] 'flutter' command not found. Please ensure Flutter SDK is in your PATH.
        popd & exit /b 1
    )

    echo.
    echo [GUI] Installing Flutter dependencies...
    call flutter pub get
    if errorlevel 1 popd & exit /b 1

    echo.
    echo [GUI] Building Flutter for Windows...
    set "FLUTTER_BUILD_MODE=debug"
    if /i "%BUILD_TYPE%"=="Release" set "FLUTTER_BUILD_MODE=release"
    set "FLUTTER_PLATFORM_DIR=x64"
    if /i "%ARCH%"=="arm64" (
        set "FLUTTER_PLATFORM_DIR=arm64"
    )
    call flutter build windows --%FLUTTER_BUILD_MODE%
    if errorlevel 1 popd & exit /b 1

    echo.
    echo [GUI] Copying artifacts...
    set "SOURCE_PATH=%PROJECT_ROOT%\server_gui\build\windows\%FLUTTER_PLATFORM_DIR%\runner\%BUILD_TYPE%"
    set "DEST_PATH=%OUTPUT_DIR%\windows-%ARCH%\"

    if not exist "%SOURCE_PATH%" (
        echo [ERROR] Flutter build artifacts not found at %SOURCE_PATH%
        popd & exit /b 1
    )

    echo [GUI] Copying from %SOURCE_PATH% to %DEST_PATH%
    xcopy "%SOURCE_PATH%" "%DEST_PATH%" /E /I /Y /Q
    if errorlevel 1 (
        echo [ERROR] Failed to copy GUI artifacts.
        popd & exit /b 1
    )

    echo [GUI] GUI build successful.
    popd
    goto :eof


:: ============================================================================
:: Help Message
:: ============================================================================
:ShowHelp
    echo.
    echo Windows Build Script for Mouse Hero Project
    echo.
    echo Usage: build_windows.bat [options]
    echo.
    echo Options:
    echo   --release          Build in Release mode.
    echo   --debug            Build in Debug mode (default is Debug).
    echo   --clean            Clean all build directories before starting.
    echo   --no-server        Skip building the server component.
    echo   --no-gui           Skip building the GUI component.
    echo   --arch ARCH        Specify architecture (x86_64 or arm64). Default: x86_64.
    echo   --msys2 PATH       Specify a custom path to the MSYS2 installation (default is C:\msys64).
    echo   -h, --help         Show this help message.
    echo.
    echo Example:
    echo   build_windows.bat --release --msys2 C:\msys64
    goto :eof
