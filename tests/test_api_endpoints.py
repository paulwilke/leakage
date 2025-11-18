"""
ESPHome API Endpoint Tests

Diese Tests validieren die JSON API Endpoints der ESP32 Sensoren:
- /sensor - Numerische Sensorwerte
- /text_sensor - Text-basierte Sensorwerte  
- /binary_sensor - Binäre Sensorwerte

Die Tests können sowohl mit Mock-Daten (ohne Hardware) als auch 
gegen echte Hardware laufen.
"""

import pytest
import requests
from typing import Dict, List, Any


# =============================================================================
# Test Configuration
# =============================================================================

# Hardware IP-Adressen (für Live-Tests)
# ANPASSEN: Setze hier die tatsächlichen IPs deiner ESP32-Geräte ein
HUMIDITY_SENSOR_IP = "10.10.50.91"
LEAK_SENSOR_IP = "10.10.50.91"  # Gleiche IP wenn nur ein Gerät zum Testen

# Timeout für API-Requests
API_TIMEOUT = 5  # Sekunden


# =============================================================================
# Fixtures
# =============================================================================

@pytest.fixture
def humidity_base_url():
    """Base URL für Humidity Sensor"""
    return f"http://{HUMIDITY_SENSOR_IP}"


@pytest.fixture
def leak_base_url():
    """Base URL für Leak Sensor"""
    return f"http://{LEAK_SENSOR_IP}"


@pytest.fixture
def mock_sensor_response():
    """Mock Response für /sensor Endpoint"""
    return [
        {
            "id": "humidity01-temperatur",
            "name": "Temperatur",
            "value": 23.4,
            "unit": "°C"
        },
        {
            "id": "humidity01-luftfeuchtigkeit",
            "name": "Luftfeuchtigkeit",
            "value": 45.2,
            "unit": "%"
        },
        {
            "id": "humidity01-wifi_signal",
            "name": "WiFi Signal",
            "value": -67.0,
            "unit": "dBm"
        },
        {
            "id": "humidity01-uptime",
            "name": "Uptime",
            "value": 3654.5,
            "unit": "s"
        }
    ]


@pytest.fixture
def mock_text_sensor_response():
    """Mock Response für /text_sensor Endpoint"""
    return [
        {
            "id": "humidity01-ip_adresse",
            "name": "IP Adresse",
            "value": "192.168.1.100"
        },
        {
            "id": "humidity01-mac_adresse",
            "name": "MAC Adresse",
            "value": "AA:BB:CC:DD:EE:FF"
        },
        {
            "id": "humidity01-ssid",
            "name": "SSID",
            "value": "TestWiFi"
        }
    ]


@pytest.fixture
def mock_binary_sensor_response():
    """Mock Response für /binary_sensor Endpoint"""
    return [
        {
            "id": "leak01-wasserleck_erkannt",
            "name": "Wasserleck erkannt",
            "value": False
        },
        {
            "id": "leak01-status",
            "name": "Status",
            "value": True
        }
    ]


# =============================================================================
# Helper Functions
# =============================================================================

def validate_sensor_data_structure(data: List[Dict[str, Any]]) -> None:
    """
    Validiert die Struktur der Sensordaten von /sensor Endpoint
    
    Args:
        data: Liste von Sensor-Daten Dictionaries
        
    Raises:
        AssertionError: Wenn Struktur ungültig ist
    """
    assert isinstance(data, list), "Response muss eine Liste sein"
    assert len(data) > 0, "Response darf nicht leer sein"
    
    for sensor in data:
        assert isinstance(sensor, dict), "Jeder Sensor muss ein Dictionary sein"
        assert "id" in sensor, "Sensor muss 'id' Feld haben"
        assert "name" in sensor, "Sensor muss 'name' Feld haben"
        assert "value" in sensor, "Sensor muss 'value' Feld haben"
        assert "unit" in sensor, "Sensor muss 'unit' Feld haben"
        
        # Datentyp-Validierung
        assert isinstance(sensor["id"], str), "ID muss String sein"
        assert isinstance(sensor["name"], str), "Name muss String sein"
        assert isinstance(sensor["value"], (int, float)), "Value muss numerisch sein"
        assert isinstance(sensor["unit"], str), "Unit muss String sein"


def validate_text_sensor_structure(data: List[Dict[str, Any]]) -> None:
    """
    Validiert die Struktur der Text-Sensordaten von /text_sensor Endpoint
    
    Args:
        data: Liste von Text-Sensor Dictionaries
        
    Raises:
        AssertionError: Wenn Struktur ungültig ist
    """
    assert isinstance(data, list), "Response muss eine Liste sein"
    assert len(data) > 0, "Response darf nicht leer sein"
    
    for sensor in data:
        assert isinstance(sensor, dict), "Jeder Sensor muss ein Dictionary sein"
        assert "id" in sensor, "Sensor muss 'id' Feld haben"
        assert "name" in sensor, "Sensor muss 'name' Feld haben"
        assert "value" in sensor, "Sensor muss 'value' Feld haben"
        
        # Datentyp-Validierung
        assert isinstance(sensor["id"], str), "ID muss String sein"
        assert isinstance(sensor["name"], str), "Name muss String sein"
        assert isinstance(sensor["value"], str), "Value muss String sein"


def validate_binary_sensor_structure(data: List[Dict[str, Any]]) -> None:
    """
    Validiert die Struktur der Binär-Sensordaten von /binary_sensor Endpoint
    
    Args:
        data: Liste von Binär-Sensor Dictionaries
        
    Raises:
        AssertionError: Wenn Struktur ungültig ist
    """
    assert isinstance(data, list), "Response muss eine Liste sein"
    assert len(data) > 0, "Response darf nicht leer sein"
    
    for sensor in data:
        assert isinstance(sensor, dict), "Jeder Sensor muss ein Dictionary sein"
        assert "id" in sensor, "Sensor muss 'id' Feld haben"
        assert "name" in sensor, "Sensor muss 'name' Feld haben"
        assert "value" in sensor, "Sensor muss 'value' Feld haben"
        
        # Datentyp-Validierung
        assert isinstance(sensor["id"], str), "ID muss String sein"
        assert isinstance(sensor["name"], str), "Name muss String sein"
        assert isinstance(sensor["value"], bool), "Value muss Boolean sein"


def validate_sensor_values(data: List[Dict[str, Any]]) -> None:
    """
    Validiert dass die Sensorwerte in plausiblen Bereichen liegen
    
    Args:
        data: Liste von Sensor-Daten
    """
    for sensor in data:
        sensor_id = sensor["id"]
        value = sensor["value"]
        
        # WiFi Signal Validierung (-100 dBm bis 0 dBm) - ZUERST prüfen!
        if "wifi" in sensor_id.lower():
            assert -100 <= value <= 0, f"WiFi Signal {value}dBm außerhalb gültigem Bereich"
        # Temperatur Validierung (-40°C bis +125°C für SHT4x)
        elif "temperatur" in sensor_id.lower() or "temperature" in sensor_id.lower():
            assert -40 <= value <= 125, f"Temperatur {value}°C außerhalb gültigem Bereich"
        # Luftfeuchtigkeit Validierung (0% bis 100%)
        elif "luftfeuchtigkeit" in sensor_id.lower() or "humidity" in sensor["name"].lower():
            assert 0 <= value <= 100, f"Luftfeuchtigkeit {value}% außerhalb gültigem Bereich"
        # Uptime Validierung (muss positiv sein)
        elif "uptime" in sensor_id.lower():
            assert value >= 0, f"Uptime {value}s muss positiv sein"


# =============================================================================
# Mock Tests (ohne Hardware)
# =============================================================================

class TestMockEndpoints:
    """Tests mit Mock-Daten (keine Hardware benötigt)"""
    
    def test_mock_sensor_structure(self, mock_sensor_response):
        """Test: /sensor Response hat korrekte Struktur"""
        validate_sensor_data_structure(mock_sensor_response)
    
    def test_mock_sensor_values(self, mock_sensor_response):
        """Test: /sensor Werte sind in plausiblen Bereichen"""
        validate_sensor_values(mock_sensor_response)
    
    def test_mock_text_sensor_structure(self, mock_text_sensor_response):
        """Test: /text_sensor Response hat korrekte Struktur"""
        validate_text_sensor_structure(mock_text_sensor_response)
    
    def test_mock_binary_sensor_structure(self, mock_binary_sensor_response):
        """Test: /binary_sensor Response hat korrekte Struktur"""
        validate_binary_sensor_structure(mock_binary_sensor_response)
    
    def test_mock_sensor_ids_unique(self, mock_sensor_response):
        """Test: Sensor IDs sind eindeutig"""
        ids = [sensor["id"] for sensor in mock_sensor_response]
        assert len(ids) == len(set(ids)), "Sensor IDs müssen eindeutig sein"


# =============================================================================
# Live Hardware Tests (nur wenn Hardware erreichbar ist)
# =============================================================================

@pytest.mark.hardware
class TestHumidityHardware:
    """Tests gegen echten Humidity Sensor (Hardware benötigt)"""
    
    def test_sensor_endpoint_reachable(self, humidity_base_url):
        """Test: /sensor Endpoint ist erreichbar"""
        try:
            response = requests.get(f"{humidity_base_url}/sensor", timeout=API_TIMEOUT)
            assert response.status_code == 200, "Endpoint sollte HTTP 200 zurückgeben"
        except requests.exceptions.RequestException as e:
            pytest.skip(f"Hardware nicht erreichbar: {e}")
    
    def test_sensor_endpoint_structure(self, humidity_base_url):
        """Test: /sensor Response hat korrekte Struktur"""
        try:
            response = requests.get(f"{humidity_base_url}/sensor", timeout=API_TIMEOUT)
            data = response.json()
            validate_sensor_data_structure(data)
        except requests.exceptions.RequestException as e:
            pytest.skip(f"Hardware nicht erreichbar: {e}")
    
    def test_sensor_endpoint_values(self, humidity_base_url):
        """Test: /sensor Werte sind plausibel"""
        try:
            response = requests.get(f"{humidity_base_url}/sensor", timeout=API_TIMEOUT)
            data = response.json()
            validate_sensor_values(data)
        except requests.exceptions.RequestException as e:
            pytest.skip(f"Hardware nicht erreichbar: {e}")
    
    def test_text_sensor_endpoint(self, humidity_base_url):
        """Test: /text_sensor Endpoint funktioniert"""
        try:
            response = requests.get(f"{humidity_base_url}/text_sensor", timeout=API_TIMEOUT)
            assert response.status_code == 200
            data = response.json()
            validate_text_sensor_structure(data)
        except requests.exceptions.RequestException as e:
            pytest.skip(f"Hardware nicht erreichbar: {e}")
    
    def test_binary_sensor_endpoint(self, humidity_base_url):
        """Test: /binary_sensor Endpoint funktioniert"""
        try:
            response = requests.get(f"{humidity_base_url}/binary_sensor", timeout=API_TIMEOUT)
            assert response.status_code == 200
            data = response.json()
            validate_binary_sensor_structure(data)
        except requests.exceptions.RequestException as e:
            pytest.skip(f"Hardware nicht erreichbar: {e}")


@pytest.mark.hardware
class TestLeakHardware:
    """Tests gegen echten Leak Sensor (Hardware benötigt)"""
    
    def test_sensor_endpoint_reachable(self, leak_base_url):
        """Test: /sensor Endpoint ist erreichbar"""
        try:
            response = requests.get(f"{leak_base_url}/sensor", timeout=API_TIMEOUT)
            assert response.status_code == 200
        except requests.exceptions.RequestException as e:
            pytest.skip(f"Hardware nicht erreichbar: {e}")
    
    def test_binary_sensor_endpoint(self, leak_base_url):
        """Test: /binary_sensor Endpoint funktioniert"""
        try:
            response = requests.get(f"{leak_base_url}/binary_sensor", timeout=API_TIMEOUT)
            assert response.status_code == 200
            data = response.json()
            validate_binary_sensor_structure(data)
            
            # Prüfe ob Leak-Sensor dabei ist
            leak_sensors = [s for s in data if "wasserleck" in s["id"].lower()]
            assert len(leak_sensors) > 0, "Leak Sensor sollte vorhanden sein"
        except requests.exceptions.RequestException as e:
            pytest.skip(f"Hardware nicht erreichbar: {e}")
    
    def test_text_sensor_endpoint(self, leak_base_url):
        """Test: /text_sensor Endpoint funktioniert"""
        try:
            response = requests.get(f"{leak_base_url}/text_sensor", timeout=API_TIMEOUT)
            assert response.status_code == 200
            data = response.json()
            validate_text_sensor_structure(data)
        except requests.exceptions.RequestException as e:
            pytest.skip(f"Hardware nicht erreichbar: {e}")

