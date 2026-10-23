@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "ACTION=%~1"
if not defined ACTION set "ACTION=debug"

if /I "%ACTION%"=="debug" goto debug
if /I "%ACTION%"=="release" goto release
if /I "%ACTION%"=="test" goto test
if /I "%ACTION%"=="run" goto run
if /I "%ACTION%"=="assets" goto assets
if /I "%ACTION%"=="help" goto help
if /I "%ACTION%"=="--help" goto help
if /I "%ACTION%"=="-h" goto help

echo Unknown command: %ACTION%
echo.
goto help_error

:debug
call :locate_tools || exit /b 1
call :build Debug debug ON
exit /b %ERRORLEVEL%

:release
call :locate_tools || exit /b 1
call :build Release release OFF
exit /b %ERRORLEVEL%

:test
call :locate_tools || exit /b 1
call :build Debug debug ON || exit /b 1
"%CTEST_EXE%" --test-dir "%BUILD_DIR%" --output-on-failure
exit /b %ERRORLEVEL%

:run
shift
call :locate_tools || exit /b 1
call :build Debug debug ON || exit /b 1
pushd "%ROOT%"
"%BUILD_DIR%\dkracing.exe" --windowed %*
set "RUN_RESULT=%ERRORLEVEL%"
popd
exit /b %RUN_RESULT%

:assets
shift
python "%ROOT%\tools\assets.py" build %*
exit /b %ERRORLEVEL%

:help
echo Usage: build-windows.bat [command] [arguments]
echo.
echo   debug       Configure and build Debug in build\debug ^(default^)
echo   release     Configure and build Release in build\release
echo   test        Build Debug and run the CTest suite
echo   run [args]  Build Debug and run dkracing.exe --windowed [args]
echo   assets [ROM] Extract runtime assets from the US 1.1 ROM
echo   help        Show this help
exit /b 0

:help_error
call :help
exit /b 1

:build
set "BUILD_DIR=%ROOT%\build\%~2"
echo Configuring %~1 build in "%BUILD_DIR%"...
"%CMAKE_EXE%" -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja "-DCMAKE_MAKE_PROGRAM=%NINJA_EXE%" "-DCMAKE_C_COMPILER=%GCC_EXE%" "-DCMAKE_BUILD_TYPE=%~1" "-DDKR_BUILD_TESTS=%~3"
if errorlevel 1 exit /b %ERRORLEVEL%

if not defined NUMBER_OF_PROCESSORS set "NUMBER_OF_PROCESSORS=4"
"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel %NUMBER_OF_PROCESSORS%
if errorlevel 1 exit /b %ERRORLEVEL%

call :stage_assets
echo Built "%BUILD_DIR%\dkracing.exe"
exit /b 0

:stage_assets
if not exist "%ROOT%\assets\assets.bin" exit /b 0
if not exist "%ROOT%\assets\assets.lut.bin" exit /b 0
if not exist "%BUILD_DIR%\assets" mkdir "%BUILD_DIR%\assets" || exit /b 1
copy /Y "%ROOT%\assets\assets.bin" "%BUILD_DIR%\assets\assets.bin" >nul || exit /b 1
copy /Y "%ROOT%\assets\assets.lut.bin" "%BUILD_DIR%\assets\assets.lut.bin" >nul || exit /b 1
exit /b 0

:locate_tools
where cmake.exe >nul 2>nul || goto search_msys2
where ninja.exe >nul 2>nul || goto search_msys2
where gcc.exe >nul 2>nul || goto search_msys2
for %%I in (cmake.exe) do set "CMAKE_EXE=%%~$PATH:I"
for %%I in (ninja.exe) do set "NINJA_EXE=%%~$PATH:I"
for %%I in (gcc.exe) do set "GCC_EXE=%%~$PATH:I"
goto tools_found

:search_msys2
if defined MSYS2_ROOT call :try_mingw "%MSYS2_ROOT%\mingw64\bin"
if defined CMAKE_EXE goto tools_found
call :try_mingw "C:\tools\msys64\mingw64\bin"
if defined CMAKE_EXE goto tools_found
call :try_mingw "C:\msys64\mingw64\bin"
if defined CMAKE_EXE goto tools_found
call :try_mingw "%USERPROFILE%\scoop\apps\msys2\current\mingw64\bin"
if defined CMAKE_EXE goto tools_found
call :try_mingw "%ProgramFiles%\MSYS2\mingw64\bin"
if defined CMAKE_EXE goto tools_found

echo Could not find the MSYS2 MINGW64 CMake, Ninja, and GCC tools.
echo Install them as described in docs\building-windows.md, or add mingw64\bin to PATH.
exit /b 1

:try_mingw
if not exist "%~1\cmake.exe" exit /b 0
if not exist "%~1\ninja.exe" exit /b 0
if not exist "%~1\gcc.exe" exit /b 0
set "PATH=%~1;%PATH%"
set "CMAKE_EXE=%~1\cmake.exe"
set "NINJA_EXE=%~1\ninja.exe"
set "GCC_EXE=%~1\gcc.exe"
exit /b 0

:tools_found
for %%I in ("%CMAKE_EXE%") do set "CTEST_EXE=%%~dpIctest.exe"
exit /b 0
