[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$IdentityName,
    [Parameter(Mandatory)][string]$Publisher,
    [string]$PublisherDisplayName = 'FuchsLabs',
    [string]$DisplayName = 'ScreenBuddy',
    [string]$Version,
    [string]$Executable = "$PSScriptRoot/../../cpp/dist/ScreenBuddy.exe",
    [string]$OutputDirectory = "$PSScriptRoot/../../dist/windows",
    [string]$CertificateThumbprint
)
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($DisplayName)) { throw 'A nonblank reserved product DisplayName is required.' }
if (!(Test-Path -LiteralPath $Executable)) { throw "Build the release EXE first: $Executable" }
$exe = (Resolve-Path -LiteralPath $Executable).Path
$actual = [Diagnostics.FileVersionInfo]::GetVersionInfo($exe).FileVersion
if (!$Version) { $Version = ([version]$actual).ToString() }
if ($Version -notmatch '^\d+\.\d+\.\d+\.0$') { throw 'Store version must have four numeric components and end in .0' }
if ([version]$actual -ne [version]$Version) { throw "EXE version $actual differs from package version $Version" }
$sdk = Get-ChildItem "${env:ProgramFiles(x86)}/Windows Kits/10/bin" -Directory |
    Where-Object { $_.Name -match '^10\.' -and (Test-Path "$($_.FullName)/x64/makeappx.exe") } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$sdk) { throw 'Windows SDK MakeAppx not found' }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$out = (Resolve-Path -LiteralPath $OutputDirectory).Path
# Fresh staging directory; never include user configuration, logs, keys or local plugins.
$stage = Join-Path $out ('stage-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path "$stage/Assets" -Force | Out-Null
Copy-Item -LiteralPath $exe -Destination "$stage/ScreenBuddy.exe"
$startup = Join-Path (Split-Path -Parent $exe) 'ScreenBuddyStartup.exe'
if (!(Test-Path -LiteralPath $startup)) { throw 'Build ScreenBuddyStartup.exe alongside ScreenBuddy.exe' }
Copy-Item -LiteralPath $startup -Destination "$stage/ScreenBuddyStartup.exe"
Copy-Item -Path "$PSScriptRoot/Assets/*.png" -Destination "$stage/Assets"
Copy-Item -LiteralPath "$PSScriptRoot/../../LICENSE" -Destination "$stage/LICENSE.txt"
Copy-Item -LiteralPath "$PSScriptRoot/../../THIRD-PARTY-NOTICES.md" -Destination "$stage/THIRD-PARTY-NOTICES.md"
$manifest = Get-Content -LiteralPath "$PSScriptRoot/AppxManifest.xml.in" -Raw
foreach ($item in @{IDENTITY=$IdentityName;PUBLISHER=$Publisher;PUBLISHER_DISPLAY=$PublisherDisplayName;DISPLAY_NAME=$DisplayName;VERSION=$Version}.GetEnumerator()) {
    $manifest = $manifest.Replace("@$($item.Key)@", [Security.SecurityElement]::Escape($item.Value))
}
[IO.File]::WriteAllText("$stage/AppxManifest.xml", $manifest, [Text.UTF8Encoding]::new($false))
$package = Join-Path $out "ScreenBuddy-$Version-x64.msix"
& "$($sdk.FullName)/x64/makeappx.exe" pack /d $stage /p $package /o
if ($LASTEXITCODE) { throw 'MSIX validation/packaging failed' }
if ($CertificateThumbprint) {
    & "$($sdk.FullName)/x64/signtool.exe" sign /sha1 $CertificateThumbprint /fd SHA256 $package
    if ($LASTEXITCODE) { throw 'MSIX signing failed' }
    & "$($sdk.FullName)/x64/signtool.exe" verify /pa $package
    if ($LASTEXITCODE) { throw 'MSIX signature verification failed' }
}
Get-FileHash -Algorithm SHA256 -LiteralPath $package | Format-List
Write-Output $package
