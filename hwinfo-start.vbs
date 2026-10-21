' (c) 2026 FuchsLabs - HWiNFO unsichtbar (neu) starten, ohne Konsolenfenster.
' Die kostenlose HWiNFO-Version beendet den Shared Memory nach 12 Stunden;
' die Aufgabe "FuchsLabs HWiNFO" startet HWiNFO deshalb alle 11 Stunden neu.
Set sh = CreateObject("WScript.Shell")
sh.Run "taskkill /IM HWiNFO64.EXE /F", 0, True
WScript.Sleep 1500
sh.Run """C:\Program Files\HWiNFO64\HWiNFO64.EXE""", 0, False
