@echo off
rem Entwickler-Hilfe: nur kompilieren (ohne Tests/dist), Ziel optional: tools\b.bat [ziel]
setlocal
cd /d "%~dp0.."
set "PATH=%PATH:"=%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if "%~1"=="" (cmake --build build) else (cmake --build build --target %1)
