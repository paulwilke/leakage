"""
pytest Konfiguration für ESPHome API Tests
"""

import pytest


def pytest_addoption(parser):
    """Fügt Command Line Optionen hinzu"""
    parser.addoption(
        "--hardware",
        action="store_true",
        default=False,
        help="Führt Hardware-Tests gegen echte ESP32-Geräte aus"
    )


def pytest_configure(config):
    """pytest Konfiguration"""
    config.addinivalue_line(
        "markers", "hardware: markiert Tests die echte Hardware benötigen"
    )

