@echo off
rem FuchsLabs - HWiNFO als sparsame Sensorquelle fuer die Systemansicht einrichten (als Administrator ausfuehren)
cd /d "%~dp0"
set "INI=C:\Program Files\HWiNFO64\HWiNFO64.INI"
if not exist "%INI%" (echo HWiNFO64 nicht gefunden. & pause & exit /b 1)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ini='%INI%'; $want=[ordered]@{SensorsOnly='1';SensorsSM='1';MinimalizeSensors='1';MinimalizeMainWnd='1';MinimalizeSensorsClose='1';ShowWelcomeAndProgress='0';SensorInterval='2000';AutoUpdate='0';AutoUpdateBetaDisable='1'};" ^
  "$l=[Collections.Generic.List[string]](Get-Content $ini); $s=$l.IndexOf('[Settings]'); if($s -lt 0){$l.Add('[Settings]'); $s=$l.Count-1};" ^
  "$e=$s+1; while($e -lt $l.Count -and -not $l[$e].StartsWith('[')){$e++};" ^
  "foreach($k in $want.Keys){ $i=-1; for($j=$s+1;$j -lt $e;$j++){ if($l[$j] -like ($k+'=*')){$i=$j} }; if($i -ge 0){$l[$i]=$k+'='+$want[$k]} else {$l.Insert($e,$k+'='+$want[$k]); $e++} };" ^
  "Set-Content -Path $ini -Value $l -Encoding ASCII; Write-Host 'HWiNFO-Einstellungen gesetzt.'"

rem Bei Anmeldung und danach alle 11 Stunden unsichtbar (neu) starten, mit Adminrechten ohne UAC-Abfrage
schtasks /Create /F /TN "FuchsLabs HWiNFO" /SC ONLOGON /RL HIGHEST /DELAY 0000:20 ^
  /TR "wscript.exe \"%~dp0hwinfo-start.vbs\"" >nul
powershell -NoProfile -Command ^
  "$t=Get-ScheduledTask -TaskName 'FuchsLabs HWiNFO'; $t.Triggers[0].Repetition=(New-ScheduledTaskTrigger -Once -At 00:00 -RepetitionInterval (New-TimeSpan -Hours 11)).Repetition; $t.Settings.DisallowStartIfOnBatteries=$false; $t.Settings.StopIfGoingOnBatteries=$false; $t.Settings.ExecutionTimeLimit='PT0S'; Set-ScheduledTask -InputObject $t | Out-Null"
echo Autostart "FuchsLabs HWiNFO" eingerichtet.

schtasks /Run /TN "FuchsLabs HWiNFO" >nul
echo HWiNFO laeuft jetzt unsichtbar im Hintergrund.
timeout /t 5
