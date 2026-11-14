[CmdletBinding()]
param([string]$SigningProperties = $env:SCREENBUDDY_SIGNING_PROPERTIES,
      [string]$OutputDirectory = "$PSScriptRoot/../dist/android")
$ErrorActionPreference = 'Stop'
if (!$SigningProperties -or !(Test-Path -LiteralPath $SigningProperties)) { throw 'Set SCREENBUDDY_SIGNING_PROPERTIES to the existing private signing properties file.' }
$env:SCREENBUDDY_SIGNING_PROPERTIES = (Resolve-Path -LiteralPath $SigningProperties).Path
if (!$env:ANDROID_HOME) { $env:ANDROID_HOME = "$env:LOCALAPPDATA/Android/sdk" }
Push-Location $PSScriptRoot
try {
    & ./gradlew.bat :protocol:test :app:lintRelease :app:assembleRelease --console=plain --max-workers=2
    if ($LASTEXITCODE) { throw 'Companion build/tests failed' }
    $sdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { "$env:LOCALAPPDATA/Android/sdk" }
    $tools = Get-ChildItem "$sdk/build-tools" -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
    if (!$tools) { throw 'Android Build Tools not found' }
    $apk = "$PSScriptRoot/app/build/outputs/apk/release/app-release.apk"
    & "$($tools.FullName)/apksigner.bat" verify --verbose --print-certs $apk
    if ($LASTEXITCODE) { throw 'APK signature verification failed' }
    New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
    $destination = Join-Path $OutputDirectory 'ScreenBuddy-Companion-1.1.1.apk'
    Copy-Item -LiteralPath $apk -Destination $destination -Force
    $hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  ScreenBuddy-Companion-1.1.1.apk" | Set-Content -LiteralPath (Join-Path $OutputDirectory 'SHA256SUMS.txt') -Encoding ascii
    Write-Output $destination
} finally { Pop-Location }
