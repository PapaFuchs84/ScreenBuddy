[CmdletBinding()]
param([Parameter(Mandatory)][string]$IdentityFile,
      [string]$BuildDirectory = "$PSScriptRoot/../../cpp/build/screenbuddy-release",
      [string]$OutputDirectory = "$PSScriptRoot/../../dist/store-submission")
$ErrorActionPreference = 'Stop'
$identity = Get-Content -LiteralPath $IdentityFile -Raw | ConvertFrom-Json
foreach ($key in @('IdentityName','Publisher','PublisherDisplayName')) {
    if (!$identity.$key -or $identity.$key -match 'REPLACE_|LocalValidation|\.Test$|ScreenBuddy Development') { throw "Official Partner Center identity required: $key" }
}
if ($identity.Publisher -match 'OID\.2\.25\.311729368913984317654407730594956997722') { throw 'Unsigned local-testing publisher is not accepted for Store submission.' }
& "$PSScriptRoot/Build-Msix.ps1" -IdentityName $identity.IdentityName -Publisher $identity.Publisher -PublisherDisplayName $identity.PublisherDisplayName -Executable (Join-Path $BuildDirectory 'ScreenBuddy.exe') -OutputDirectory $OutputDirectory
Copy-Item -LiteralPath "$PSScriptRoot/STORE-LISTING.md" -Destination (Join-Path $OutputDirectory 'STORE-LISTING.md') -Force
Get-ChildItem -LiteralPath $OutputDirectory -Filter '*.msix' | ForEach-Object {
    $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $($_.Name)" | Set-Content -LiteralPath (Join-Path $OutputDirectory 'SHA256SUMS.txt') -Encoding ascii
}
