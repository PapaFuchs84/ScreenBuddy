# © 2026 FuchsLabs – adapted from the isolated 2.10.0 test launcher.
# Fixed UI 18766 / AI 18765, no installer, no display hardware.
param([string]$DataDirectory, [switch]$AiReceiver)
$ErrorActionPreference = 'Stop'
$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (-not $DataDirectory) { $DataDirectory = Join-Path $workspace 'cpp\build\game-mode-test\data' }
$data = [IO.Path]::GetFullPath($DataDirectory)
if (-not $data.StartsWith($workspace + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Test data must stay inside this workspace.' }
$exe = Join-Path $workspace 'cpp\build\screenbuddy-release\ScreenBuddy.exe'
$cfg = Join-Path $data 'config.toml'
if (-not (Test-Path -LiteralPath $cfg)) { throw 'Prepare an isolated config.toml before launching.' }
$configText = Get-Content -LiteralPath $cfg -Raw
if ($configText -notmatch '(?ms)^\[ui\]\s*\r?\n(?:(?!^\[).)*?^port\s*=\s*18766\s*$') { throw 'Test UI must use port 18766.' }
foreach ($port in @(18766) + $(if ($AiReceiver) { @(18765) } else { @() })) {
    $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, $port)
    try { $listener.Start() } finally { $listener.Stop() }
}
if (-not $AiReceiver -and $configText -match '(?ms)^\[ai\]\s*\r?\n(?:(?!^\[).)*?^enabled\s*=\s*true\s*$') { throw 'AI receiver needs -AiReceiver and a free port 18765.' }
$env:FUCHSLABS_INSTANCE = 'gamemode18766'
$env:FUCHSLABS_INSTALL_DIR = $data
$env:FUCHSLABS_LEGACY_DIR = Join-Path $data 'missing-legacy'
$args2 = @('--run', '--no-lcd', '-c', ('"' + $cfg + '"'), '--log', ('"' + (Join-Path $data 'test.log') + '"'))
$p = Start-Process -FilePath $exe -ArgumentList $args2 -WorkingDirectory $workspace -WindowStyle Hidden -PassThru
@{ pid = $p.Id; ui_port = 18766; ai_port = 18765; data = $data } | ConvertTo-Json -Compress
