param([string]$Lang='de')
$s=$PSScriptRoot; $out="$s\final2\$Lang"; if(Test-Path $out){Remove-Item $out -Recurse -Force}; New-Item -ItemType Directory -Force $out | Out-Null
$null=& pwsh -NoProfile -File "$s\start.ps1" -Lang $Lang
$id=(Get-Process ScreenBuddy | ? Path -like "$s*").Id
& pwsh -NoProfile -File "$s\resize.ps1" -ProcId $id; Start-Sleep 2
$tab=@{ de=@{ki=271;game=388;spot=1278;views=1405}; en=@{ki=276;game=397;spot=1297;views=1415} }[$Lang]
function Nav($x,$y,$n,$w=1800){ & pwsh -NoProfile -File "$s\go.ps1" -X $x -Y $y -Name "..\final2\$Lang\$n" -Wait $w | Out-Null }
function Scroll($d,$n){ & pwsh -NoProfile -File "$s\scroll.ps1" -ProcId $id -X 740 -Y 520 -Delta $d -Out "$out\$n.png" | Out-Null }
function Click1($x,$y,$n,$w=2200){
  $prev=$null; for($i=0;$i -lt 3;$i++){
    & pwsh -NoProfile -File "$s\click.ps1" -ProcId $id -X $x -Y $y -Out "$out\$n.png" -Wait $w | Out-Null
    $h=(Get-FileHash "$out\$n.png").Hash; if($i -eq 0){ $before=$script:last } ; if($h -ne $script:last){ $script:last=$h; return } }
  Write-Warning "no change after click: $n" }
$script:last=''
Nav 56 164 01-uebersicht; $script:last=(Get-FileHash "$out\01-uebersicht.png").Hash
Nav 56 236 02-anzeige
Scroll -700 03-sperrzone
Nav 714 205 04-seiten-reihenfolge
Nav 1245 205 05-design
Nav 56 308 x1; Remove-Item "$out\x1.png"
Nav $tab.ki 205 06-ki-status
Nav $tab.game 205 07-game-mode
Scroll -1500 08-game-erkennung
Nav $tab.spot 205 09-spotify
Nav $tab.views 205 10-eigene-ansichten 2500
$script:last=(Get-FileHash "$out\10-eigene-ansichten.png").Hash
Click1 292 523 11-ansicht-game-mode
Nav 285 630 12-zonen-editor-hilfe 2500; $script:last=(Get-FileHash "$out\12-zonen-editor-hilfe.png").Hash
Click1 208 536 13-zonen-editor
"done"

