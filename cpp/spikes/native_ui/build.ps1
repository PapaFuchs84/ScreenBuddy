param([string] $CoreBuildDir = "")
$ErrorActionPreference = 'Stop'
$cppRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (!$CoreBuildDir) { $CoreBuildDir = Join-Path $cppRoot 'build' }
$coreLibrary = Join-Path $CoreBuildDir 'flcore.lib'
if (!(Test-Path -LiteralPath $coreLibrary)) { throw 'Build flcore.lib first. This script never builds or runs the production app.' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsRoot) { throw 'MSVC x64 Build Tools not found.' }
$vcVersion = (Get-Content -LiteralPath (Join-Path $vsRoot 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
$vcRoot = Join-Path $vsRoot "VC\Tools\MSVC\$vcVersion"
$kitRoot = (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots').KitsRoot10
$kitVersion = (Get-ChildItem -LiteralPath (Join-Path $kitRoot 'Include') -Directory | Where-Object Name -Match '^10\.' | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1).Name
if (!$kitVersion) { throw 'Windows 10/11 C++ SDK not found.' }
$compiler = Join-Path $vcRoot 'bin\Hostx64\x64\cl.exe'
$deps = Join-Path $cppRoot 'vcpkg_installed\x64-windows-static'
$outputDir = Join-Path $CoreBuildDir 'native-ui-spike'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$includes = @((Join-Path $vcRoot 'include'), (Join-Path $kitRoot "Include\$kitVersion\ucrt"), (Join-Path $kitRoot "Include\$kitVersion\shared"), (Join-Path $kitRoot "Include\$kitVersion\um"), (Join-Path $kitRoot "Include\$kitVersion\winrt"), (Join-Path $cppRoot 'src'), (Join-Path $deps 'include'))
$arguments = @('/nologo', '/std:c++20', '/utf-8', '/W4', '/EHsc', '/permissive-', '/MT', '/O2', '/DUNICODE', '/D_UNICODE', '/DNOMINMAX', '/DWIN32_LEAN_AND_MEAN', '/D_WIN32_WINNT=0x0A00')
$arguments += $includes | ForEach-Object { "/I$_" }
$arguments += @((Join-Path $PSScriptRoot 'native_settings_spike.cpp'), "/Fo$(Join-Path $outputDir 'native_settings_spike.obj')", "/Fe$(Join-Path $outputDir 'ScreenBuddy-Native-UI-Spike.exe')", '/link', '/SUBSYSTEM:WINDOWS', '/MANIFEST:EMBED', "/MANIFESTINPUT:$(Join-Path $PSScriptRoot 'native_settings_spike.manifest')", '/OPT:REF', '/OPT:ICF', '/DYNAMICBASE', '/NXCOMPAT', '/HIGHENTROPYVA', "/LIBPATH:$(Join-Path $vcRoot 'lib\x64')", "/LIBPATH:$(Join-Path $kitRoot "Lib\$kitVersion\ucrt\x64")", "/LIBPATH:$(Join-Path $kitRoot "Lib\$kitVersion\um\x64")", "/LIBPATH:$(Join-Path $deps 'lib')", $coreLibrary)
$arguments += @('tomlplusplus.lib', 'pugixml.lib', 'libssl.lib', 'libcrypto.lib', 'd2d1.lib', 'dwrite.lib', 'windowscodecs.lib', 'winhttp.lib', 'ws2_32.lib', 'iphlpapi.lib', 'pdh.lib', 'crypt32.lib', 'setupapi.lib', 'shlwapi.lib', 'comctl32.lib', 'dwmapi.lib', 'bcrypt.lib', 'advapi32.lib', 'user32.lib', 'gdi32.lib', 'ole32.lib', 'shell32.lib', 'uuid.lib')
$previousPath = $env:PATH
try {
    # link.exe locates the SDK manifest tool through PATH even when cl.exe has an absolute path.
    $env:PATH = "$(Join-Path $kitRoot "bin\$kitVersion\x64");$(Join-Path $vcRoot 'bin\Hostx64\x64');$previousPath"
    & $compiler @arguments
    if ($LASTEXITCODE -ne 0) { throw "Compile/link failed ($LASTEXITCODE)." }
} finally { $env:PATH = $previousPath }
Write-Output "Compiled and linked only; NOT executed: $(Join-Path $outputDir 'ScreenBuddy-Native-UI-Spike.exe')"
