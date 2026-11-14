# Datenschutz für ScreenBuddy und ScreenBuddy Companion

Stand: 10. November 2026. Diese Erklärung beschreibt ScreenBuddy für Windows und die separat erhältliche Android-Companion-App. Die Verarbeitung beim Besuch unserer Website steht in der [Website-Datenschutzerklärung](https://fuchslabs.duckdns.org/datenschutz/).

## Verantwortlicher und Kontakt

FuchsLabs, Sebastian Herbst. Anschrift und weitere Anbieterangaben stehen im [Impressum](https://fuchslabs.duckdns.org/impressum/). Datenschutz- und Supportanfragen: [fuchslabs2026@gmail.com](mailto:fuchslabs2026@gmail.com).

## Lokaler Betrieb und gespeicherte Daten

ScreenBuddy benötigt kein FuchsLabs-Konto. Einstellungen, Seitenlayouts, Themes, Plugins, Kopplungen und Diagnoseprotokolle werden auf deinem Gerät gespeichert. Die Store-Ausgabe verwendet den Windows-Paketdatenordner `LocalState`; die Standalone-Ausgabe einen eigenen lokalen Datenordner. Zugangswerte in den dafür vorgesehenen Einstellungen werden mit Windows DPAPI an das Windows-Benutzerkonto gebunden. Diese Verschlüsselung ersetzt keinen Schutz deines angemeldeten Windows-Kontos.

Systemanzeigen lesen beispielsweise Prozessor-, Grafik-, Speicher-, Temperatur-, Datenträger- und Netzwerkmesswerte. Die optionale Pegel-/Spektrumanzeige verarbeitet die aktuelle Windows-Audioausgabe lokal über Loopback zur Berechnung von Pegeln und Frequenzen; sie ist keine Mikrofon- oder Sprachaufnahmefunktion. ScreenBuddy betreibt keinen eigenen zentralen Analyse-, Werbe- oder automatischen Diagnose-Uploaddienst. Das bedeutet nicht, dass alle Funktionen offline arbeiten.

## Verbindungen und optionale Dienste

Bei Netzwerkverbindungen können Zielanbieter insbesondere IP-Adresse, Anfragezeit, angefragte Inhalte und die für den Dienst erforderlichen Kennungen oder Zugangsdaten erhalten. Es gelten zusätzlich die Bedingungen und Datenschutzhinweise des jeweiligen Anbieters; dessen Verarbeitungsorte können außerhalb der EU liegen.

- **Updates und Katalog:** Microsoft verteilt Updates der Store-Ausgabe und verarbeitet dabei Daten nach seinen eigenen Bedingungen. Die Store-Ausgabe fragt keine GitHub-App-Releases ab. Die Standalone-Ausgabe kann bei aktivierter Updateprüfung GitHub kontaktieren. Der Plugin-/Theme-Katalog lädt Kataloge und Pakete von den konfigurierten HTTPS-Hosts; er ist über die App-Einstellungen abschaltbar.
- **Freunde und Discord:** Eingerichtete Integrationen können Konto-/Spielerkennungen, Anzeigenamen, Avatare, Freundeslisten, Präsenz- und Spielinformationen bei Discord, Steam, dem Xbox-Dienst über OpenXBL (`xbl.io`) sowie PlayStation Network abfragen. Die hierfür eingetragenen Schlüssel oder Tokens werden für die ausgewählte Verbindung verwendet. Ohne die erforderliche Einrichtung stehen diese Dienste nicht zur Verfügung.
- **Musik, Streaming und RSS:** Spotify verarbeitet die eingerichtete Anmeldung und Anfragen zum Wiedergabestatus; Cover werden vom Spotify-Bilddienst geladen. Streaming-Neustarts verwenden mit deinem API-Schlüssel TMDB, einschließlich Region und ausgewählter Anbieter. RSS lädt die von dir eingetragenen Feed-Adressen; Inhalte und Links können von fremden Anbietern stammen.
- **Smart Home:** Die optionale Home-Assistant-Verbindung überträgt Anfragen mit dem eingetragenen Token an deine konfigurierte Instanz und zeigt ausgewählte Entitätszustände. Die gewählte HTTP-/HTTPS-Adresse und die Sicherheit dieser Instanz liegen in deiner Kontrolle.
- **Game-Mode:** Die lokale Erkennung liest installierte Spiele aus Launcher-Dateien/Registry und laufende Prozesse (Name, Pfad, Prozess-ID). Sie verändert keine Spielprozesse und verwendet keine Injektion oder Spielprozess-Speicherauslesung. ETW bzw. HWiNFO liefern Leistungswerte. Spielname und Vordergrunddateiname können in lokalen Diagnoseprotokollen erscheinen. Spielelogos können unter Verwendung der Steam-App-ID vom Steam-CDN `shared.fastly.steamstatic.com` geladen und lokal zwischengespeichert werden. Eine konfigurierte Ping-Messung sendet Netzwerkpakete an das gewählte Ziel.
- **Battlefield-Statistiken:** Eingerichtete Spielernamen bzw. Spielerkennungen werden für Statistikabfragen an `api.gametools.network` gesendet.
- **Plugins und Links:** Installierte Plugins können die von ihnen deklarierten und von dir freigegebenen Netzwerkquellen verwenden. Freigegebene Link-Aktionen öffnen den Browser. Prüfe Berechtigungen und Zieladressen vor dem Einschalten.

## KI-Status und Nutzungsanzeigen

KI-Status ist optional. Nach deiner bestätigten Einrichtung melden lokale Claude-Code-/Codex-Hooks Sitzungskennung, Ereignis, Toolname und gegebenenfalls Modell an einen authentifizierten Empfänger auf `127.0.0.1`. Der Standardadapter leitet keine Prompts, Toolargumente oder Transkripte weiter. Statuswerte liegen im Arbeitsspeicher; Hook-Dateien, Adapter, verschlüsselter Schlüssel und Sicherungen bleiben lokal gespeichert, bis du sie entfernst.

Die getrennten Schalter zur Nutzung echter Konten sind standardmäßig aus und erfordern eine Bestätigung. Erst dann kann ScreenBuddy vorhandene lokale Anmeldedateien von Claude oder Codex lesen und deren Zugriffstoken über HTTPS ausschließlich an den jeweiligen Anbieter senden: Anthropic (`api.anthropic.com`) bzw. OpenAI (`chatgpt.com`). Dies sind inoffizielle, veränderliche Nutzungsabfragen. ScreenBuddy speichert diese Tokens nicht zusätzlich, schreibt die Anmeldedateien nicht und erneuert keine Tokens. Bei deaktivierter Kontonutzung werden keine echten Konto-Anmeldedateien dafür verwendet. Ausschalten beendet weitere Abfragen; bereits laufende Anfragen können innerhalb ihrer Zeitbegrenzung noch abgeschlossen werden.

## Companion und deine Anzeigen

Nach ausdrücklicher Kopplung sendet der PC gerenderte Anzeigeseiten über TLS an das ausgewählte Companion-Gerät. Darin können Informationen aus deinen aktivierten Integrationen enthalten sein. Touch-/Navigationsbefehle und Geräteabmessungen werden zurückgesendet. Achte darauf, wer die angeschlossenen Bildschirme sehen kann.

Die Android-App speichert unter anderem PC-Adresse, Port, Fingerabdruck, Gerätename und Kopplungsschlüssel im app-eigenen Speicher; der Schlüssel wird über Android Keystore geschützt. Die optionale Kamera wird zur lokalen QR-Code-Erkennung bei der Einrichtung verwendet. „PC vergessen“ entfernt die gespeicherte Verbindung und den Schlüssel. Das App-Manifest deaktiviert Android-Backup; allgemeine Aussagen zu jedem Hersteller-Geräteübertragungsverfahren werden daraus nicht abgeleitet.

## Optionale Companion-Automatik (ab 1.1.0)

Wenn deine separat installierte Companion-Version diese Funktion anbietet, ist sie zunächst ausgeschaltet und benötigt eine vorhandene Kopplung. Sie sucht den gekoppelten PC über lokales mDNS und kann dessen gespeicherten Companion-Port im privaten Netz mit einem TCP-Verbindungsaufbau auf Erreichbarkeit prüfen; dieser reine Verbindungsversuch überträgt keinen Kopplungscode und keine Anzeigedaten. Die aktivierte Bereitschaft kann als Android-Vordergrunddienst mit Benachrichtigung laufen und nach einem Geräteneustart wieder starten. Die Suche kann zusätzliche Energie benötigen.

Zum Öffnen aus dem Hintergrund kann die App eine optionale Freigabe zum Anzeigen über anderen Apps anbieten. Auf unterstützten Handys kann sie nach einer getrennten Android-Geräteadmin-Freigabe den Bildschirm sperren. Sie hebt keine vorhandene PIN-/Biometriesperre auf. Android-TV bietet diese Geräteadmin-Sperre nicht an; Systemverhalten beim Aufwecken kann je nach Gerät HDMI-CEC auslösen. Benachrichtigungs-, Overlay- und Geräteadmin-Freigaben werden in Android verwaltet. Schalte die Automatik in der App aus und widerrufe erteilte Systemfreigaben bei Bedarf dort; vor einer Deinstallation kann das Deaktivieren der Geräteadmin-Freigabe nötig sein. Manuelle Nutzung bleibt ohne Automatik möglich.

## Kontrolle, Aufbewahrung und Löschen

Du kannst Integrationen abschalten, Zugangswerte entfernen, Kopplungen aufheben, Plugins deinstallieren und lokale App-Daten löschen. Aufbewahrte Einstellungen, Caches, Sicherungen und lokale Protokolle haben keinen zentralen FuchsLabs-Löschzeitplan; sie bleiben abhängig von Funktion und Betriebssystem lokal bestehen, bis sie ersetzt oder gelöscht werden. Entferne KI-Hooks über die Vorschau-/Entfernungsfunktion, bevor du ihre Dateien oder die App-Daten löschst. Das bloße Abschalten einer Anzeige löscht keine bestehenden Dateien. Ein Import aus der Standalone-Ausgabe erhält die Quelldateien ausdrücklich.

Wenn du Support anforderst, sendest du die von dir ausgewählten Angaben an FuchsLabs. Sende keine Passwörter, Tokens, Kopplungscodes oder unbereinigten Konfigurationen. Wir verwenden die übermittelten Informationen zur Bearbeitung deiner Anfrage und bewahren sie nur so lange auf, wie dafür oder wegen gesetzlicher Pflichten erforderlich. Wir verkaufen diese Supportdaten nicht.

## Rechtsgrundlagen und Rechte

Soweit FuchsLabs personenbezogene Daten für die Bereitstellung einer angefragten Leistung oder die Bearbeitung einer Anfrage verarbeitet, erfolgt dies auf Grundlage von Art. 6 Abs. 1 lit. b DSGVO bzw. des berechtigten Interesses an Support und sicherem Betrieb nach Art. 6 Abs. 1 lit. f DSGVO. Freiwillig freigegebene zusätzliche Verarbeitung kann auf einer Einwilligung nach Art. 6 Abs. 1 lit. a DSGVO beruhen; diese kann für die Zukunft widerrufen werden. Die Verarbeitung durch eingerichtete Drittanbieter unterliegt zusätzlich deren eigenen Rechtsgrundlagen.

Nach Maßgabe der DSGVO bestehen insbesondere Rechte auf Auskunft, Berichtigung, Löschung, Einschränkung, Datenübertragbarkeit und Widerspruch sowie auf Beschwerde bei einer Datenschutzaufsichtsbehörde. Für Anfragen an FuchsLabs nutze den Kontakt oben; bei einem Drittanbieter gespeicherte Daten musst du gegebenenfalls direkt dort verwalten lassen.
