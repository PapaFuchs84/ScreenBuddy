[CmdletBinding()]
param([string]$BuildDirectory = "$PSScriptRoot/../../cpp/build/store",
      [string]$OutputDirectory = "$PSScriptRoot/../../cpp/build/msix-validation")
$ErrorActionPreference = 'Stop'
$identity = 'FuchsLabs.ScreenBuddy.LocalValidation'
if (Get-AppxPackage -Name $identity) { throw 'Previous validation package exists; review it first.' }
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$out = (Resolve-Path -LiteralPath $OutputDirectory).Path
$publisher = 'CN=ScreenBuddy Local Validation'
$installed = $null
function Invoke-Probe([string]$name, [switch]$ExerciseStartup) {
    $report = Join-Path $out "$name.json"
    if (Test-Path -LiteralPath $report) { throw 'Use a fresh output directory.' }
    $arguments = "packaged `"$report`""
    if ($ExerciseStartup) { $arguments += ' exercise-startup' }
    Invoke-CommandInDesktopPackage -PackageFamilyName $installed.PackageFamilyName -AppId ScreenBuddy -Command "$build/fl-distribution-tests.exe" -Args $arguments -PreventBreakaway
    $deadline = (Get-Date).AddSeconds(30)
    while (!(Test-Path -LiteralPath $report) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 200 }
    if (!(Test-Path -LiteralPath $report)) { throw "Probe failed: $name" }
    $data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    if (!$data.packaged -or $data.channel -ne 'microsoft-store') { throw 'Wrong package context.' }
    return $data
}
try {
    # Developer registration supports StartupTask without installing certificate trust.
    # This checks package context and local deployment; it is not signed Store servicing.
    & "$PSScriptRoot/Build-Msix.ps1" -IdentityName $identity -Publisher $publisher -Executable "$build/ScreenBuddy.exe" -OutputDirectory $out
    $msix = Get-ChildItem -LiteralPath $out -Filter '*.msix' | Select-Object -First 1
    $stage = Get-ChildItem -LiteralPath $out -Directory -Filter 'stage-*' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    Add-AppxPackage -Register "$($stage.FullName)/AppxManifest.xml"
    $installed = Get-AppxPackage -Name $identity
    if (!$installed -or $installed.Status -ne 'Ok') { throw 'Installation failed.' }
    $before = Invoke-Probe 'before-update' -ExerciseStartup
    $marker = Join-Path $before.data_dir 'distribution-test-marker.txt'
    if ((Get-Content -LiteralPath $marker -Raw) -ne 'package-data-persistence') { throw 'LocalState write failed.' }
    $source = Join-Path $out 'example-standalone'
    New-Item -ItemType Directory -Force -Path "$source/themes","$source/plugin-settings" | Out-Null
    "[ui]`nlanguage = 'en'`nupdate_check = false" | Set-Content -LiteralPath "$source/config.toml" -Encoding utf8
    '{"example":true}' | Set-Content -LiteralPath "$source/companion.json"
    'example-theme' | Set-Content -LiteralPath "$source/themes/example.fltheme.json"
    'example-settings' | Set-Content -LiteralPath "$source/plugin-settings/example.json"
    & "$PSScriptRoot/Import-StandaloneData.ps1" -PackageFamilyName $installed.PackageFamilyName -SourceDirectory $source -WhatIf
    if (Test-Path -LiteralPath (Join-Path $before.data_dir 'config.toml')) { throw 'WhatIf changed data.' }
    & "$PSScriptRoot/Import-StandaloneData.ps1" -PackageFamilyName $installed.PackageFamilyName -SourceDirectory $source
    foreach ($relative in @('config.toml','companion.json','themes/example.fltheme.json','plugin-settings/example.json')) {
        if ((Get-FileHash -LiteralPath (Join-Path $source $relative)).Hash -ne (Get-FileHash -LiteralPath (Join-Path $before.data_dir $relative)).Hash) { throw "Import mismatch: $relative" }
    }
    $refused = $false
    try { & "$PSScriptRoot/Import-StandaloneData.ps1" -PackageFamilyName $installed.PackageFamilyName -SourceDirectory $source } catch { $refused = $true }
    if (!$refused) { throw 'Existing Store configuration was overwritten.' }
    # Synthetic package-only update: candidate EXEs are unchanged.
    $stage = Get-ChildItem -LiteralPath $out -Directory -Filter 'stage-*' | Select-Object -First 1
    [xml]$manifest = Get-Content -LiteralPath "$($stage.FullName)/AppxManifest.xml" -Raw
    $v = [version]$manifest.Package.Identity.Version
    $next = [version]::new($v.Major,$v.Minor,$v.Build + 1,0)
    $manifest.Package.Identity.Version = $next.ToString()
    $manifest.Save("$($stage.FullName)/AppxManifest.xml")
    $sdk = Get-ChildItem "${env:ProgramFiles(x86)}/Windows Kits/10/bin" -Directory | Where-Object { $_.Name -match '^10\.' -and (Test-Path "$($_.FullName)/x64/makeappx.exe") } | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
    $upgrade = Join-Path $out 'Synthetic-Upgrade.msix'
    & "$($sdk.FullName)/x64/makeappx.exe" pack /d $stage.FullName /p $upgrade /o
    if ($LASTEXITCODE) { throw 'Update packaging failed.' }
    Add-AppxPackage -Register "$($stage.FullName)/AppxManifest.xml"
    $installed = Get-AppxPackage -Name $identity
    if ([version]$installed.Version -ne $next) { throw 'Version did not update.' }
    if ((Get-Content -LiteralPath $marker -Raw) -ne 'package-data-persistence') { throw 'Update lost data.' }
    $after = Invoke-Probe 'after-update'
    if ($before.data_dir -ne $after.data_dir) { throw 'Data directory changed.' }
    if ((Get-FileHash -LiteralPath "$source/config.toml").Hash -ne (Get-FileHash -LiteralPath (Join-Path $after.data_dir 'config.toml')).Hash) { throw 'Update changed configuration.' }
    @{deploymentMode="developer-registration";install=$true;startupRoundtrip=$true;import=$true;whatIf=$true;overwriteRefused=$true;update=$true;dataRetained=$true;syntheticPackageVersion=$next.ToString();storeCertification=$false} | ConvertTo-Json | Set-Content -LiteralPath "$out/results.json"
} finally {
    if ($installed) { Remove-AppxPackage -Package $installed.PackageFullName }
}
if (Get-AppxPackage -Name $identity) { throw 'Test uninstall failed.' }
Write-Output "MSIX developer registration, startup, import, synthetic update, persistence and uninstall passed: $out/results.json"
