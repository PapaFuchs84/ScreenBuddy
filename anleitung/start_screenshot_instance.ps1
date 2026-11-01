param(
    [string]$Build = 'cpp/build/screenbuddy-dev',
    [ValidateSet('de', 'en')][string]$Language = 'de',
    [ValidateRange(1024,65535)][int]$Port = 18955
)
$ErrorActionPreference = 'Stop'
if ($Port -eq 8765 -or $Port -eq 8777) { throw 'Use a dedicated screenshot port.' }
if (Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue) {
    throw "Screenshot port $Port is already in use. Do not reuse another instance."
}
$buildPath = (Resolve-Path -LiteralPath $Build).Path
$runtime = Join-Path $buildPath ('screenshot-runtime-' + $Language)
New-Item -ItemType Directory -Force -Path $runtime | Out-Null
# Copy the binary so a live capture session never prevents the next build link.
$captureExe = Join-Path $runtime 'ScreenBuddy.exe'
Copy-Item -LiteralPath (Join-Path $buildPath 'ScreenBuddy.exe') -Destination $captureExe
$fixture = @"
[ui]
port = $Port
setup_done = true
language = "$Language"
update_check = false
settings_theme = "hell"
[display]
port = "COM255"
output = "serial"
page_seconds = 15
theme = "dunkel"
[system]
enabled = false
[discord]
enabled = false
[friends]
enabled = false
[bf6]
enabled = false
[rss]
enabled = false
[homeassistant]
enabled = false
[companion]
enabled = false
port = 18957
"@
$configPath = Join-Path $runtime 'screenshot-demo.toml'
[IO.File]::WriteAllText($configPath, $fixture, [Text.UTF8Encoding]::new($false))
$previousInstance = $env:FUCHSLABS_INSTANCE
$previousInstallDir = $env:FUCHSLABS_INSTALL_DIR
try {
    $env:FUCHSLABS_INSTANCE = "release-screenshots-$Language-$Port"
    $env:FUCHSLABS_INSTALL_DIR = $runtime
    $captureArguments = '--run --no-lcd --settings --config "' + $configPath + '" --log "' + (Join-Path $runtime 'capture.log') + '"'
    $captureProcess = Start-Process -FilePath $captureExe -ArgumentList $captureArguments -WindowStyle Hidden -PassThru
} finally {
    $env:FUCHSLABS_INSTANCE = $previousInstance
    $env:FUCHSLABS_INSTALL_DIR = $previousInstallDir
}
$baseUrl = "http://127.0.0.1:$Port"
$ready = $false
for ($attempt = 0; $attempt -lt 40; $attempt++) {
    try {
        $metadata = Invoke-RestMethod ($baseUrl + '/api/config') -TimeoutSec 1
        $ready = $true
        break
    } catch { Start-Sleep -Milliseconds 250 }
}
if (!$ready) { throw "Screenshot instance did not become ready (PID $($captureProcess.Id))." }
Invoke-RestMethod -Method Post -Headers @{'X-FL'='1'} ($baseUrl + '/api/open-window') | Out-Null
[pscustomobject]@{
    Pid = $captureProcess.Id
    Version = $metadata.version
    Language = $Language
    Port = $Port
    BinarySHA256 = (Get-FileHash -LiteralPath $captureExe -Algorithm SHA256).Hash
    Quit = $baseUrl + '/api/quit'
    HardwareEvidence = $false
}
