[CmdletBinding()]
param(
    [Parameter(Mandatory)][string[]]$Path,
    [string]$CertificateThumbprint = $env:SCREENBUDDY_CODESIGN_THUMBPRINT,
    [ValidateSet('CurrentUser','LocalMachine')][string]$CertificateStore = 'CurrentUser',
    [uri]$TimestampUrl = 'http://timestamp.digicert.com'
)
$ErrorActionPreference = 'Stop'
if (!$CertificateThumbprint) { throw 'A publicly trusted code-signing certificate is required. Set SCREENBUDDY_CODESIGN_THUMBPRINT. No unsigned public release is permitted.' }
$thumbprint = $CertificateThumbprint.Replace(' ', '')
$cert = Get-Item -LiteralPath "Cert:/$CertificateStore/My/$thumbprint" -ErrorAction Stop
if (!$cert.HasPrivateKey -or $cert.NotAfter -le (Get-Date) -or $cert.NotBefore -gt (Get-Date)) { throw 'Certificate is expired, not yet valid, or has no accessible private key.' }
if ($cert.Subject -eq $cert.Issuer) { throw 'Self-signed certificates are not accepted for public releases.' }
if (!($cert.Extensions | Where-Object { $_.Oid.Value -eq '2.5.29.37' } | ForEach-Object { $_.EnhancedKeyUsages } | Where-Object { $_.Value -eq '1.3.6.1.5.5.7.3.3' })) { throw 'Certificate lacks the Code Signing extended key usage.' }
$sdk = Get-ChildItem "${env:ProgramFiles(x86)}/Windows Kits/10/bin" -Directory |
    Where-Object { $_.Name -match '^10\.' -and (Test-Path "$($_.FullName)/x64/signtool.exe") } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$sdk) { throw 'Windows SDK SignTool not found.' }
$files = @($Path | ForEach-Object { (Resolve-Path -LiteralPath $_).Path })
foreach ($file in $files) {
    if ([IO.Path]::GetExtension($file) -notin @('.exe','.dll','.msi','.msix','.appx')) { throw "Unsupported signing input: $file" }
}
foreach ($file in $files) {
    $arguments = @('sign','/sha1',$thumbprint,'/s','My','/fd','SHA256','/tr',$TimestampUrl.AbsoluteUri,'/td','SHA256')
    if ($CertificateStore -eq 'LocalMachine') { $arguments += '/sm' }
    & "$($sdk.FullName)/x64/signtool.exe" @arguments $file
    if ($LASTEXITCODE) { throw "Signing failed: $file" }
    & "$($sdk.FullName)/x64/signtool.exe" verify /pa /all /tw $file
    if ($LASTEXITCODE) { throw "Authenticode verification failed: $file" }
    $signature = Get-AuthenticodeSignature -LiteralPath $file
    if ($signature.Status -ne 'Valid' -or !$signature.TimeStamperCertificate -or $signature.SignerCertificate.Thumbprint -ne $thumbprint) {
        throw "A valid timestamped signature from the configured publisher is required: $file"
    }
    Write-Output "Verified timestamped Authenticode signature: $([IO.Path]::GetFileName($file))"
}
