[CmdletBinding()]
param([Parameter(Mandatory)][string]$IdentityFile,
      [string]$BuildDirectory = "$PSScriptRoot/../../cpp/build/screenbuddy-release",
      [string]$OutputDirectory = "$PSScriptRoot/../../dist/store-submission")
$ErrorActionPreference = 'Stop'
$identity = Get-Content -LiteralPath $IdentityFile -Raw | ConvertFrom-Json
foreach ($key in @('IdentityName','Publisher','PublisherDisplayName')) {
    if ($identity.$key -isnot [string] -or [string]::IsNullOrWhiteSpace($identity.$key) -or
        $identity.$key -match 'REPLACE_|LocalValidation|\.Test$|ScreenBuddy Development') { throw "Official Partner Center identity required: $key" }
}
if ($identity.Publisher -match 'OID\.2\.25\.311729368913984317654407730594956997722') { throw 'Unsigned local-testing publisher is not accepted for Store submission.' }
$displayName = 'ScreenBuddy'
if ($identity.PSObject.Properties.Name -contains 'DisplayName') {
    if ($identity.DisplayName -isnot [string] -or [string]::IsNullOrWhiteSpace($identity.DisplayName) -or $identity.DisplayName -match 'REPLACE_') {
        throw 'Reserved Partner Center product DisplayName required.'
    }
    $displayName = $identity.DisplayName
}
foreach ($assetDirectory in @('Screenshots', 'ListingAssets')) {
    if (Test-Path -LiteralPath (Join-Path $OutputDirectory $assetDirectory)) {
        throw "Submission assets already exist: $assetDirectory. Select a fresh -OutputDirectory; existing files will not be deleted or merged."
    }
}
$buildOutput = @(& "$PSScriptRoot/Build-Msix.ps1" -IdentityName $identity.IdentityName -Publisher $identity.Publisher -PublisherDisplayName $identity.PublisherDisplayName -DisplayName $displayName -Executable (Join-Path $BuildDirectory 'ScreenBuddy.exe') -OutputDirectory $OutputDirectory)
# Build-Msix emits the exact package path last. Never hash older packages that
# happen to remain in a reused output directory.
$package = Get-Item -LiteralPath ([string]$buildOutput[-1])
if ($package.Extension -ne '.msix' -or $package.PSIsContainer -or
    $package.Directory.FullName -ne (Resolve-Path -LiteralPath $OutputDirectory).Path) {
    throw 'Build-Msix did not return a package in the submission output directory.'
}
Copy-Item -LiteralPath "$PSScriptRoot/STORE-LISTING.md" -Destination (Join-Path $OutputDirectory 'STORE-LISTING.md') -Force
foreach ($document in @('APP-PRIVACY.de.md', 'APP-PRIVACY.en.md', 'APP-SUPPORT.de.md', 'APP-SUPPORT.en.md', 'STORE-FORMS-DRAFT.md', 'CERTIFICATION-NOTES.md')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $document) -Destination (Join-Path $OutputDirectory $document) -Force
}
foreach ($assetDirectory in @('Screenshots', 'ListingAssets')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $assetDirectory) -Destination $OutputDirectory -Recurse -Force
}
$hash = (Get-FileHash -LiteralPath $package.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $($package.Name)" | Set-Content -LiteralPath (Join-Path $OutputDirectory 'SHA256SUMS.txt') -Encoding ascii
Write-Output $package.FullName
