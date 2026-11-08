param([string]$Lang='de')
$s=$PSScriptRoot; $out="$s\final2\$Lang"
$null=& pwsh -NoProfile -File "$s\start.ps1" -Lang $Lang
$id=(Get-Process ScreenBuddy | ? Path -like "$s*").Id
& pwsh -NoProfile -File "$s\resize.ps1" -ProcId $id; Start-Sleep 2
$tab=@{ de=@{fr=502;dc=598;bf=708;bl=831;st=987;ha=1157}; en=@{fr=512;dc=609;bf=723;bl=851;st=1003;ha=1171} }[$Lang]
function Nav($x,$y,$n,$w=1800){ & pwsh -NoProfile -File "$s\go.ps1" -X $x -Y $y -Name "..\final2\$Lang\$n" -Wait $w | Out-Null }
Nav 56 308 x1; Remove-Item "$out\x1.png"
Nav $tab.fr 205 14-freunde
Nav $tab.dc 205 15-discord
Nav $tab.bf 205 16-battlefield
Nav $tab.bl 205 17-blogs
Nav $tab.st 205 18-streaming
Nav $tab.ha 205 19-homeassistant
Nav 56 380 20-erweiterungen
Nav 56 452 21-app
"done"
