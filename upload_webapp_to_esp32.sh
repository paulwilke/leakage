#!/bin/bash

# ============================================================================
# Upload Leak Webapp zum ESP32
# ============================================================================
# Kompiliert die Firmware, flasht sie und lädt die Webapp hoch
# ============================================================================

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}ESP32 Leak Sensor - Firmware & Webapp Upload${NC}"
echo -e "${BLUE}================================================${NC}\n"

# Aktiviere venv
source venv/bin/activate

# Schritt 1: Kompiliere und flashe Firmware
echo -e "${BLUE}Schritt 1: Kompiliere Firmware...${NC}"
if esphome compile leak_sensors/leak01.yaml; then
    echo -e "${GREEN}✓ Firmware kompiliert${NC}\n"
else
    echo -e "${RED}✗ Kompilierung fehlgeschlagen!${NC}"
    exit 1
fi

echo -e "${BLUE}Schritt 2: Flashe Firmware auf ESP32...${NC}"
echo -e "${YELLOW}Wähle Upload-Methode:${NC}"
echo -e "${YELLOW}  [1] USB (wenn per Kabel verbunden)${NC}"
echo -e "${YELLOW}  [2] OTA (wenn bereits geflasht und im Netzwerk)${NC}"
read -p "Methode (1 oder 2): " method

if [ "$method" = "1" ]; then
    echo -e "${BLUE}Flashe per USB...${NC}"
    esphome upload leak_sensors/leak01.yaml
elif [ "$method" = "2" ]; then
    echo -e "${BLUE}Flashe per OTA auf 10.10.50.91...${NC}"
    esphome upload leak_sensors/leak01.yaml --device 10.10.50.91
else
    echo -e "${RED}Ungültige Auswahl!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Firmware geflasht${NC}\n"

# Schritt 3: Warte bis ESP32 neu gestartet ist
echo -e "${YELLOW}Warte 10 Sekunden bis ESP32 neu gestartet ist...${NC}"
sleep 10

# Schritt 4: Upload Webapp
echo -e "${BLUE}Schritt 3: Lade Webapp hoch...${NC}"
echo -e "${YELLOW}Die Webapp wird jetzt auf http://10.10.50.91/ hochgeladen${NC}\n"

# Prüfe ob curl verfügbar ist
if ! command -v curl &> /dev/null; then
    echo -e "${RED}curl ist nicht installiert!${NC}"
    exit 1
fi

# Upload der HTML-Datei per HTTP POST zum ESP32 Webserver
# ESPHome's Webserver v2 unterstützt File-Upload per /upload endpoint
echo -e "${BLUE}Uploading leak_webapp.html...${NC}"

# Copy webapp to root as index.html (wird als Standard-Seite dienen)
curl -X POST -F "file=@esp32_webinterface/leak_webapp.html" \
     "http://10.10.50.91/upload?path=/index.html" \
     -w "\nHTTP Status: %{http_code}\n"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Webapp erfolgreich hochgeladen!${NC}\n"
else
    echo -e "${YELLOW}⚠ Upload fehlgeschlagen oder nicht unterstützt${NC}"
    echo -e "${YELLOW}Der ESP32 Webserver unterstützt möglicherweise keinen File-Upload${NC}"
    echo -e "${YELLOW}Alternativ: Webapp lokal öffnen mit ./start_webapp.sh${NC}\n"
fi

echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}✓ Setup abgeschlossen!${NC}"
echo -e "${GREEN}================================================${NC}\n"

echo -e "${BLUE}Öffne im Browser:${NC}"
echo -e "  ${GREEN}http://10.10.50.91/${NC}          - Leak Sensor Dashboard\n"

echo -e "${BLUE}Logs ansehen:${NC}"
echo -e "  ${GREEN}esphome logs 10.10.50.91${NC}\n"

echo -e "${BLUE}Tests ausführen:${NC}"
echo -e "  ${GREEN}pytest tests/ -v -m hardware${NC}\n"

