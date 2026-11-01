[CmdletBinding(SupportsShouldProcess)]
param([Parameter(Mandatory)][string]$PackageFamilyName,
      [string]$SourceDirectory = "$env:LOCALAPPDATA/FuchsLabs/USB-Display")
$ErrorActionPreference = 'Stop'
$package = Get-AppxPackage | Where-Object PackageFamilyName -eq $PackageFamilyName | Select-Object -First 1
if (!$package) { throw 'Install and launch the package once before importing.' }
$sourceRoot = [IO.Path]::GetFullPath($SourceDirectory).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
$packageRoot = [IO.Path]::GetFullPath($package.InstallLocation).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
$conflicts = Get-Process ScreenBuddy,FuchsLabs-USB-Display -ErrorAction SilentlyContinue | Where-Object {
    $_.Path -and ($_.Path.StartsWith($sourceRoot, [StringComparison]::OrdinalIgnoreCase) -or $_.Path.StartsWith($packageRoot, [StringComparison]::OrdinalIgnoreCase))
}
if ($conflicts) { throw 'Close the source and destination ScreenBuddy instances before import.' }
$destination = Join-Path $env:LOCALAPPDATA "Packages/$PackageFamilyName/LocalState"
if (!(Test-Path -LiteralPath "$SourceDirectory/config.toml")) { throw 'Source configuration not found' }
# Never overwrite initialized Store data. Source files remain untouched.
if (Test-Path -LiteralPath "$destination/config.toml") { throw 'Store configuration exists. Back it up and move it aside before importing.' }
$allowed = @('config.toml','companion.json','plugins','plugin-settings','themes')
foreach ($name in $allowed) {
    $source = Join-Path $SourceDirectory $name
    if (!(Test-Path -LiteralPath $source)) { continue }
    $items = @(Get-Item -LiteralPath $source) + @(Get-ChildItem -LiteralPath $source -Recurse -Force -ErrorAction Stop)
    if ($items | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }) { throw 'Symlinks/junctions are not imported.' }
    $target = Join-Path $destination $name
    if (Test-Path -LiteralPath $target) { throw "Target exists: $name" }
}
if ($PSCmdlet.ShouldProcess($destination, 'Copy standalone configuration, pairing data, plugins and themes')) {
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    foreach ($name in $allowed) {
        $source = Join-Path $SourceDirectory $name
        if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination (Join-Path $destination $name) -Recurse }
    }
}
