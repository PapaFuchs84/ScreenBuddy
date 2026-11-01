[CmdletBinding()]
param([string]$BuildDirectory = "$PSScriptRoot/../../cpp/build/screenbuddy-release",
      [string]$OutputDirectory = "$PSScriptRoot/../../dist/windows-public",
      [string]$CertificateThumbprint = $env:SCREENBUDDY_CODESIGN_THUMBPRINT)
$ErrorActionPreference = 'Stop'
$files = @('ScreenBuddy.exe','ScreenBuddyStartup.exe') | ForEach-Object { Join-Path $BuildDirectory $_ }
# Sign build outputs before copying or hashing any public artifact.
& "$PSScriptRoot/Sign-Windows.ps1" -Path $files -CertificateThumbprint $CertificateThumbprint
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$lines = foreach ($file in $files) {
    $destination = Join-Path $OutputDirectory ([IO.Path]::GetFileName($file))
    Copy-Item -LiteralPath $file -Destination $destination -Force
    $hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $([IO.Path]::GetFileName($destination))"
}
$lines | Set-Content -LiteralPath (Join-Path $OutputDirectory 'SHA256SUMS.txt') -Encoding ascii
