#!/bin/bash

# ============================================================================
# ESPHome Testing Script
# ============================================================================
# Dieses Script führt alle Tests für die ESPHome Sensoren aus:
# 1. Syntax-Validierung (esphome config)
# 2. Kompilierung (esphome compile)
# 3. Unit-Tests (pytest)
# 4. Optional: Hardware-Tests
#
# Usage:
#   ./run_tests.sh              # Nur Syntax, Compile und Mock-Tests
#   ./run_tests.sh --hardware   # Inkl. Hardware-Tests
# ============================================================================

set -e  # Exit bei Fehlern

# Farben für Output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Funktionen für farbigen Output
print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

# Check ob venv existiert
if [ ! -d "venv" ]; then
    print_error "Virtual environment nicht gefunden!"
    print_info "Führe zuerst aus: python3 -m venv venv && source venv/bin/activate && pip install -r requirements.txt"
    exit 1
fi

# Aktiviere venv
source venv/bin/activate

# Parse Command Line Arguments
HARDWARE_TESTS=false
if [ "$1" = "--hardware" ]; then
    HARDWARE_TESTS=true
    print_info "Hardware-Tests sind aktiviert"
fi

# Zähler für Statistik
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# ============================================================================
# 1. SYNTAX-VALIDIERUNG
# ============================================================================
print_header "1. Syntax-Validierung"

print_info "Validiere humidity01.yaml..."
if esphome config humidity_sensors/humidity01.yaml > /dev/null 2>&1; then
    print_success "humidity01.yaml ist syntaktisch korrekt"
    ((PASSED_TESTS++))
else
    print_error "humidity01.yaml hat Syntax-Fehler"
    esphome config humidity_sensors/humidity01.yaml
    ((FAILED_TESTS++))
fi
((TOTAL_TESTS++))

print_info "Validiere leak01.yaml..."
if esphome config leak_sensors/leak01.yaml > /dev/null 2>&1; then
    print_success "leak01.yaml ist syntaktisch korrekt"
    ((PASSED_TESTS++))
else
    print_error "leak01.yaml hat Syntax-Fehler"
    esphome config leak_sensors/leak01.yaml
    ((FAILED_TESTS++))
fi
((TOTAL_TESTS++))

# ============================================================================
# 2. KOMPILIERUNG
# ============================================================================
print_header "2. Kompilierung"

print_info "Kompiliere humidity01.yaml..."
print_info "(Dies kann einige Minuten dauern beim ersten Mal...)"
if esphome compile humidity_sensors/humidity01.yaml > /dev/null 2>&1; then
    print_success "humidity01.yaml erfolgreich kompiliert"
    ((PASSED_TESTS++))
else
    print_error "humidity01.yaml Kompilierung fehlgeschlagen"
    esphome compile humidity_sensors/humidity01.yaml
    ((FAILED_TESTS++))
fi
((TOTAL_TESTS++))

print_info "Kompiliere leak01.yaml..."
print_info "(Dies kann einige Minuten dauern beim ersten Mal...)"
if esphome compile leak_sensors/leak01.yaml > /dev/null 2>&1; then
    print_success "leak01.yaml erfolgreich kompiliert"
    ((PASSED_TESTS++))
else
    print_error "leak01.yaml Kompilierung fehlgeschlagen"
    esphome compile leak_sensors/leak01.yaml
    ((FAILED_TESTS++))
fi
((TOTAL_TESTS++))

# ============================================================================
# 3. UNIT-TESTS (Mock-Tests)
# ============================================================================
print_header "3. Unit-Tests (Mock-Daten)"

print_info "Führe pytest Mock-Tests aus..."
if pytest tests/ -v -m "not hardware"; then
    print_success "Alle Mock-Tests bestanden"
    ((PASSED_TESTS++))
else
    print_error "Einige Mock-Tests fehlgeschlagen"
    ((FAILED_TESTS++))
fi
((TOTAL_TESTS++))

# ============================================================================
# 4. HARDWARE-TESTS (Optional)
# ============================================================================
if [ "$HARDWARE_TESTS" = true ]; then
    print_header "4. Hardware-Tests"
    
    print_info "Führe pytest Hardware-Tests aus..."
    print_info "Stelle sicher, dass die ESP32-Geräte erreichbar sind!"
    
    if pytest tests/ -v -m "hardware"; then
        print_success "Alle Hardware-Tests bestanden"
        ((PASSED_TESTS++))
    else
        print_error "Einige Hardware-Tests fehlgeschlagen"
        ((FAILED_TESTS++))
    fi
    ((TOTAL_TESTS++))
else
    print_info "Hardware-Tests übersprungen (verwende --hardware um sie auszuführen)"
fi

# ============================================================================
# ZUSAMMENFASSUNG
# ============================================================================
print_header "Test-Zusammenfassung"

echo "Gesamt: $TOTAL_TESTS Tests"
echo -e "${GREEN}Bestanden: $PASSED_TESTS${NC}"
if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "${RED}Fehlgeschlagen: $FAILED_TESTS${NC}"
fi

if [ $FAILED_TESTS -eq 0 ]; then
    echo ""
    print_success "Alle Tests erfolgreich! ✨"
    exit 0
else
    echo ""
    print_error "Einige Tests fehlgeschlagen!"
    exit 1
fi

