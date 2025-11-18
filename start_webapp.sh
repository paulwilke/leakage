#!/bin/bash

# ============================================================================
# Start Leak Sensor Webapp
# ============================================================================
# Startet einen lokalen Webserver und öffnet die Leak Sensor Webapp
# Die Webapp greift auf den ESP32 bei 10.10.50.91 zu
# ============================================================================

# Farben
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo -e "${BLUE}Starting Leak Sensor Webapp...${NC}\n"

# Check ob ESP32 erreichbar ist (mit 2 Sekunden Timeout)
if curl -s --max-time 2 --connect-timeout 2 http://10.10.50.91/binary_sensor > /dev/null 2>&1; then
    echo -e "${GREEN}✓ ESP32 ist erreichbar unter 10.10.50.91${NC}"
else
    echo -e "${YELLOW}⚠ ESP32 nicht erreichbar!${NC}"
    echo -e "${YELLOW}  Stelle sicher dass:${NC}"
    echo -e "${YELLOW}  1. ESP32 eingeschaltet ist${NC}"
    echo -e "${YELLOW}  2. Du im gleichen Netzwerk bist${NC}"
    echo -e "${YELLOW}  3. Die Firmware hochgeladen wurde: ./upload_leak.sh${NC}\n"
fi

echo -e "${BLUE}Starte Webserver auf Port 8000...${NC}"
echo -e "${GREEN}Öffne im Browser:${NC} ${BLUE}http://localhost:8000/leak_webapp.html${NC}\n"
echo -e "${YELLOW}Zum Beenden: Strg+C drücken${NC}\n"

# Wechsel ins esp32_webinterface Verzeichnis
if [ ! -d "esp32_webinterface" ]; then
    echo -e "${YELLOW}⚠ Verzeichnis 'esp32_webinterface' nicht gefunden!${NC}"
    exit 1
fi

cd esp32_webinterface

# Starte Python Webserver (optimiert mit --bind für schnelleres Starten)
python3 -m http.server 8000 --bind 127.0.0.1

