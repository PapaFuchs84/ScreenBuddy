param([int]$X,[int]$Y,[string]$Name,[int]$Wait=1800)
$s=$PSScriptRoot; $id=(Get-Process ScreenBuddy | ? Path -like "$s*").Id
& pwsh -NoProfile -File "$s\click.ps1" -ProcId $id -X $X -Y $Y -Wait 700 | Out-Null
& pwsh -NoProfile -File "$s\click.ps1" -ProcId $id -X $X -Y $Y -Out "$s\raw\$Name.png" -Wait $Wait | Out-Null
