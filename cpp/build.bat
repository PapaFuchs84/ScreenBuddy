@echo off
rem (c) 2026 FuchsLabs - baut FuchsLabs-USB-Display.exe (eine Datei, statisch gelinkt) nach dist\
rem Aufruf: build.bat [nopause]
setlocal
cd /d "%~dp0"
set "VCPKG_ROOT=%USERPROFILE%\FuchsLabs\vcpkg"
set "VSDEV=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VSDEV%" (
  echo Visual Studio Build Tools 2022 nicht gefunden.
  goto :fail
)
rem Anfuehrungszeichen im PATH bringen vcvars64.bat aus dem Tritt
set "PATH=%PATH:"=%"
call "%VSDEV%" >nul || goto :fail
if not exist build\build.ninja (
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static -DVCPKG_INSTALLED_DIR="%~dp0vcpkg_installed" || goto :fail
)
cmake --build build || goto :fail
build\fl-tests.exe || goto :fail
if not exist dist mkdir dist
copy /y build\FuchsLabs-USB-Display.exe dist\ >nul || goto :fail
echo.
echo Fertig: dist\FuchsLabs-USB-Display.exe
if /i not "%~1"=="nopause" pause
exit /b 0
:fail
echo.
echo Bauen fehlgeschlagen.
if /i not "%~1"=="nopause" pause
exit /b 1
