@echo off
rem (c) 2026 FuchsLabs - baut ScreenBuddy.exe (eine Datei, statisch gelinkt) nach dist\
rem Aufruf: build.bat [nopause]
setlocal
cd /d "%~dp0"
set "BUILD_DIR=build\screenbuddy-release"
set "VCPKG_ROOT=%USERPROFILE%\FuchsLabs\vcpkg"
set "VSDEV=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VSDEV%" (
  echo Visual Studio Build Tools 2022 nicht gefunden.
  goto :fail
)
rem Anfuehrungszeichen im PATH bringen vcvars64.bat aus dem Tritt
"%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows-static --x-install-root "%~dp0vcpkg_installed" || goto :fail
set "PATH=%PATH:"=%"
call "%VSDEV%" >nul || goto :fail
set "CMAKE=cmake"
where cmake >nul 2>nul
if errorlevel 1 set "CMAKE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
"%CMAKE%" -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static -DVCPKG_INSTALLED_DIR="%~dp0vcpkg_installed" ^
  -DVCPKG_MANIFEST_MODE=OFF -DCMAKE_TRY_COMPILE_CONFIGURATION=Release || goto :fail
"%CMAKE%" --build "%BUILD_DIR%" || goto :fail
"%BUILD_DIR%\fl-tests.exe" || goto :fail
"%BUILD_DIR%\fl-tmdb-tests.exe" || goto :fail
"%BUILD_DIR%\fl-plugin-tests.exe" || goto :fail
"%BUILD_DIR%\fl-plugin-example-tests.exe" || goto :fail
"%BUILD_DIR%\fl-plugin-store-tests.exe" || goto :fail
"%BUILD_DIR%\fl-plugin-transport-tests.exe" || goto :fail
"%BUILD_DIR%\fl-plugin-settings-tests.exe" || goto :fail
"%BUILD_DIR%\fl-monitor-tests.exe" || goto :fail
"%BUILD_DIR%\fl-companion-tests.exe" || goto :fail
python tests\installer_preserves_config.py "%BUILD_DIR%\ScreenBuddy.exe" || goto :fail
if not exist dist mkdir dist
copy /y "%BUILD_DIR%\ScreenBuddy.exe" dist\ >nul || goto :fail
echo.
echo Fertig: dist\ScreenBuddy.exe
if /i not "%~1"=="nopause" pause
exit /b 0
:fail
echo.
echo Bauen fehlgeschlagen.
if /i not "%~1"=="nopause" pause
exit /b 1
