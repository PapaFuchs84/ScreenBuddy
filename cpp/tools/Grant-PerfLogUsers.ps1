# © 2026 FuchsLabs – ScreenBuddy: FPS-/Frametime-Messung ohne Administrator
# Fügt den aktuellen Benutzer zur lokalen Gruppe "Leistungsprotokollbenutzer" (Performance Log Users) hinzu.
# Nötig, damit ScreenBuddy die Windows-Present-Ereignisse (ETW) lesen darf. Es wird nichts in Spiele eingehängt.
# Wirkt erst nach Ab- und wieder Anmelden. Rückgängig: net localgroup "Performance Log Users" <Benutzer> /delete
# Muss einmalig als Administrator laufen (Rechtsklick > Als Administrator ausführen). Nichts wird automatisch ausgeführt.
param([string]$User = "$env:USERDOMAIN\$env:USERNAME")
$ok = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $ok) { Write-Host "Bitte als Administrator starten." -ForegroundColor Red; exit 1 }
$group = (New-Object Security.Principal.SecurityIdentifier 'S-1-5-32-559').Translate([Security.Principal.NTAccount]).Value.Split('\')[-1]
Write-Host "Füge $User zur Gruppe '$group' hinzu ..."
net localgroup "$group" "$User" /add
Write-Host "Fertig. Bitte einmal ab- und wieder anmelden, danach ScreenBuddy neu starten."
