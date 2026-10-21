@echo off
rem FuchsLabs USB-Display - EXE bauen. Legt bei Bedarf eine eigene Python-Umgebung (.venv) an.
rem Voraussetzung: Python 3.11 oder neuer von python.org (64 Bit).
cd /d "%~dp0"
if not exist .venv\Scripts\python.exe (
  py -3.13 -m venv .venv 2>nul || py -3 -m venv .venv || (echo Python 3 nicht gefunden - bitte von python.org installieren & pause & exit /b 1)
)
.venv\Scripts\python -m pip install --require-virtualenv --disable-pip-version-check -r requirements.txt -r requirements-build.txt || (pause & exit /b 1)

rem Programm-Symbol (gleiches Zeichen wie im Infobereich)
.venv\Scripts\python -c "import tray, shutil; shutil.copy(tray._make_icon(), 'app.ico')" || (pause & exit /b 1)

rem Kleiner und schneller: ungenutzte Module weglassen, Bytecode optimieren
.venv\Scripts\python -m PyInstaller --noconfirm --clean --windowed --optimize 2 --name FuchsLabs-USB-Display ^
  --icon app.ico --version-file version-info.txt --add-data "webui.html;." --add-data "ANLEITUNG.html;." --add-data "GUIDE.html;." ^
  --hidden-import serial.tools.list_ports_windows ^
  --exclude-module tkinter --exclude-module numpy --exclude-module unittest --exclude-module pydoc ^
  --exclude-module doctest --exclude-module test --exclude-module distutils --exclude-module setuptools ^
  --exclude-module xmlrpc --exclude-module pdb --exclude-module curses --exclude-module sqlite3 ^
  --exclude-module PIL._avif --exclude-module PIL.AvifImagePlugin ^
  fuchslabs_display.py || (echo Build fehlgeschlagen & pause & exit /b 1)

set "OUT=dist\FuchsLabs-USB-Display"
rem Bebilderte Anleitung: neu erzeugen mit anleitung\make_images.py (Screenshots in anleitung\bilder);
rem englisch: make_images.py <englische Screenshots> en -> GUIDE.html
copy /Y ANLEITUNG.html "%OUT%\" >nul
copy /Y GUIDE.html "%OUT%\" >nul
copy /Y README.en.md "%OUT%\" >nul
copy /Y ANLEITUNG.md "%OUT%\" >nul
copy /Y DOKUMENTATION.md "%OUT%\" >nul
copy /Y hwinfo-einrichten.bat "%OUT%\" >nul
copy /Y hwinfo-start.vbs "%OUT%\" >nul
echo.
echo Fertig: %OUT%\FuchsLabs-USB-Display.exe
if not "%1"=="nopause" pause
