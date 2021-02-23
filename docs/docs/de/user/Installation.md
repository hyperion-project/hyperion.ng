# Installiere Hyperion
Hyperion unterstützt zahlreiche Plattformen zur Installation und wird als Paket oder .zip bereitgestellt.

## Voraussetzungen

### Unterstützte Systeme
  * Raspberry Pi (Siehe [HyperBian](/en/user/HyperBian), derzeit nur auf Englisch)
  * Debian 9 | Ubuntu 16.04 oder neuer
  * Mac OS
  * Windows 10

**Einige arm Geräte haben eine reduzierte Untersützung der Funktion des mitschneidens des Bildschirms**

### Unterstützte Browser
Hyperion wird durch ein Web Interface konfiguriert und gesteuert.
  * Chrome 47+
  * Firefox 43+
  * Opera 34+
  * Safari 9.1+
  * Microsoft Edge 14+

::: warnung Internet Explorer
Internet Explorer ist nicht unterstützt.
:::

## Installation von Hyperion
  * Raspberry Pi man kann [HyperBian](/en/user/HyperBian.md) für ein neues System nutzen. Alternativ kann es auf einem bestehenden System installiert werden
  * Wir stellen Installationspakete (.deb) bereit um Hyperion mit nur einem Klick auf Debian/Ubuntu basierten Systemen zu installieren.
  * Mac OSX - derzeit nur eine Zip-Datei mit Binärdaten
  * Windows 10 - Lade die Windows-AMD.64.exe von der [Release page](https://github.com/hyperion-project/hyperion.ng/releases) herunter und installiere es.

### Debian/Ubuntu
Für Debian/Ubuntu stellen wir eine .deb Datei bereit. Damit geschieht die Installation automatisch. \
Lade die Datei von der [Release page](https://github.com/hyperion-project/hyperion.ng/releases) herunter. \
Installiere über die Kommandozeile indem du \
`sudo apt install ./Hyperion-2.0.0-Linux-x86_64.deb` \
eintippst. Hyperion kann jetzt über das Startmenü gestartet werden.

### Fedora
Für Fedora stellen wir eine .rpm Datei bereit. Damit geschieht die Installation automatisch. \
Lade die Datei von der [Release page](https://github.com/hyperion-project/hyperion.ng/releases) herunter. \
Installiere über die Kommandozeile indem du \
`sudo dnf install ./Hyperion-2.0.0-Linux-x86_64.rpm` \
eintippst. Hyperion kann jetzt über das Startmenü gestartet werden.

### Windows 10
Für Windows 10 stellen wir eine .exe Datei bereit. Damit geschieht die Installation automatisch. \
Lade die Datei von der [Release page](https://github.com/hyperion-project/hyperion.ng/releases) herunter. \
Installiere über das ausführen der .exe Datei. Hyperion kann jetzt über das Startmenü gestartet werden.

## Deinstallieren von Hyperion
In Debian/Ubuntu kann man Hyperion mit diesem Kommando deinstallieren \
`sudo apt remove hyperion` \
In Windows erfolgt die Deinstallation über die Systemsteuerung.

### Hyperion lokale Daten
Hyperion speichert lokale Daten in dem "Home"-Ordner (Ordner `.hyperion`).
