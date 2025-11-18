#!/bin/bash

# ============================================================================
# ESPHome Leak Sensor Upload Script
# ============================================================================
# Lädt die Firmware per OTA auf das Gerät bei 10.10.50.91
# ============================================================================

set -e

# Farben
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Uploading Leak Sensor Firmware per OTA...${NC}\n"

# Aktiviere venv
source venv/bin/activate

# Upload mit OTA (Option 2 automatisch wählen)
echo "2" | esphome run leak_sensors/leak01.yaml --device 10.10.50.91

echo -e "\n${GREEN}✓ Upload erfolgreich!${NC}"
echo -e "${BLUE}Logs ansehen:${NC} esphome logs 10.10.50.91"
echo -e "${BLUE}Tests ausführen:${NC} pytest tests/ -v -m hardware"

