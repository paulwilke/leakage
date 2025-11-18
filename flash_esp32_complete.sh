#!/bin/bash

# ============================================================================
# Komplettes ESP32 Setup - Firmware + Webapp
# ============================================================================
# Flasht die Firmware und erklärt wie die Webapp hochgeladen wird
# ============================================================================

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

clear
echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  ESP32 Leak Sensor - Komplettes Setup             ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}\n"

# Aktiviere venv
echo -e "${BLUE}→ Aktiviere Python venv...${NC}"
source venv/bin/activate

# Prüfe ob leak01.yaml vorhanden ist
if [ ! -f "leak_sensors/leak01.yaml" ]; then
    echo -e "${RED}✗ leak_sensors/leak01.yaml nicht gefunden!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Konfiguration gefunden${NC}\n"

# Validiere Konfiguration
echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Schritt 1: Validiere Konfiguration               ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}\n"

if esphome config leak_sensors/leak01.yaml > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Konfiguration ist valide${NC}\n"
else
    echo -e "${RED}✗ Konfigurationsfehler!${NC}"
    esphome config leak_sensors/leak01.yaml
    exit 1
fi

# Kompiliere
echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Schritt 2: Kompiliere Firmware                   ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}\n"
echo -e "${YELLOW}Dies kann beim ersten Mal einige Minuten dauern...${NC}\n"

if esphome compile leak_sensors/leak01.yaml; then
    echo -e "\n${GREEN}✓ Firmware erfolgreich kompiliert!${NC}\n"
else
    echo -e "${RED}✗ Kompilierung fehlgeschlagen!${NC}"
    exit 1
fi

# Upload
echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Schritt 3: Firmware auf ESP32 flashen            ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}\n"

echo -e "${YELLOW}Wähle Upload-Methode:${NC}"
echo -e "  ${GREEN}[1]${NC} USB (ESP32 per Kabel verbunden)"
echo -e "  ${GREEN}[2]${NC} OTA (ESP32 im Netzwerk auf 10.10.50.91)"
echo ""
read -p "Methode (1 oder 2): " method

case $method in
    1)
        echo -e "\n${BLUE}→ Flashe per USB...${NC}\n"
        esphome run leak_sensors/leak01.yaml
        ;;
    2)
        echo -e "\n${BLUE}→ Flashe per OTA auf 10.10.50.91...${NC}\n"
        esphome run leak_sensors/leak01.yaml --device 10.10.50.91
        ;;
    *)
        echo -e "${RED}Ungültige Auswahl!${NC}"
        exit 1
        ;;
esac

echo -e "\n${GREEN}✓ Firmware erfolgreich geflasht!${NC}\n"

# Warte auf Neustart
echo -e "${YELLOW}Warte 5 Sekunden bis ESP32 neu gestartet ist...${NC}\n"
sleep 5

# Test ob erreichbar
echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Schritt 4: Teste ESP32 Verbindung                ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}\n"

if curl -s http://10.10.50.91/ > /dev/null 2>&1; then
    echo -e "${GREEN}✓ ESP32 ist erreichbar auf http://10.10.50.91/${NC}\n"
else
    echo -e "${YELLOW}⚠ ESP32 noch nicht erreichbar${NC}"
    echo -e "${YELLOW}  Warte noch etwas und öffne dann: http://10.10.50.91/${NC}\n"
fi

# Finale Anleitung
echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  ✓ Setup abgeschlossen!                            ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}\n"

echo -e "${GREEN}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  Was jetzt?                                        ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════╝${NC}\n"

echo -e "${BLUE}1. Standard ESPHome Interface:${NC}"
echo -e "   ${GREEN}http://10.10.50.91/${NC}"
echo -e "   → Funktioniert SOFORT, zeigt alle Sensordaten\n"

echo -e "${BLUE}2. API direkt testen:${NC}"
echo -e "   ${GREEN}curl http://10.10.50.91/binary_sensor | jq${NC}"
echo -e "   → Zeigt JSON-Daten des Leak-Sensors\n"

echo -e "${BLUE}3. Professional Webapp (optional):${NC}"
echo -e "   Die schicke Webapp kann auf zwei Arten genutzt werden:\n"

echo -e "   ${YELLOW}Option A:${NC} Lokal öffnen (einfach)"
echo -e "   ${GREEN}./start_webapp.sh${NC}"
echo -e "   → Startet Webserver auf Port 8000"
echo -e "   → Öffne: ${GREEN}http://localhost:8000/leak_webapp.html${NC}\n"

echo -e "   ${YELLOW}Option B:${NC} Auf ESP32 hosten (komplex)"
echo -e "   → Webapp direkt auf ESP32 hochladen"
echo -e "   → Siehe: ${GREEN}esp32_webinterface/README.md${NC}\n"

echo -e "${BLUE}4. Logs ansehen:${NC}"
echo -e "   ${GREEN}esphome logs 10.10.50.91${NC}"
echo -e "   → Live-Logs vom ESP32\n"

echo -e "${BLUE}5. Funktionstest:${NC}"
echo -e "   → Verbinde die beiden Sensor-Drähte"
echo -e "   → Prüfe: ${GREEN}curl http://10.10.50.91/binary_sensor${NC}"
echo -e "   → Sollte ${GREEN}\"value\": true${NC} zeigen\n"

echo -e "${BLUE}6. Tests ausführen:${NC}"
echo -e "   ${GREEN}pytest tests/ -v -m hardware${NC}\n"

echo -e "${GREEN}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  Viel Erfolg! 🚀                                   ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════╝${NC}\n"

