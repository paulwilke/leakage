/* Kompaktes JavaScript für ESP32 Leak Sensor Dashboard */
/* Wird direkt vom ESP32 ausgeliefert und greift auf die JSON API zu */

(function(){
  // Auto-Refresh alle 3 Sekunden
  const REFRESH_INTERVAL = 3000;
  
  // Haupt-Update Funktion
  async function updateDashboard(){
    try{
      // Hole alle Sensordaten parallel
      const [binarySensors, sensors, textSensors] = await Promise.all([
        fetch('/binary_sensor').then(r=>r.json()),
        fetch('/sensor').then(r=>r.json()),
        fetch('/text_sensor').then(r=>r.json())
      ]);
      
      // Leak Status
      const leakSensor = binarySensors.find(s=>s.id.includes('wasserleck'));
      if(leakSensor){
        const isWet = leakSensor.value;
        const statusEl = document.getElementById('leak-status');
        const iconEl = document.getElementById('leak-icon');
        const textEl = document.getElementById('leak-text');
        
        if(isWet){
          statusEl.className = 'leak-status wet';
          iconEl.className = 'leak-icon wet';
          iconEl.textContent = '💧⚠️';
          textEl.className = 'leak-text wet';
          textEl.textContent = 'WASSERLECK!';
        }else{
          statusEl.className = 'leak-status dry';
          iconEl.className = 'leak-icon dry';
          iconEl.textContent = '✓';
          textEl.className = 'leak-text dry';
          textEl.textContent = 'TROCKEN';
        }
      }
      
      // WiFi Signal
      const wifiSensor = sensors.find(s=>s.id.includes('wifi'));
      if(wifiSensor){
        document.getElementById('wifi-value').textContent = wifiSensor.value.toFixed(0);
      }
      
      // Uptime
      const uptimeSensor = sensors.find(s=>s.id.includes('uptime'));
      if(uptimeSensor){
        const hours = Math.floor(uptimeSensor.value / 3600);
        const minutes = Math.floor((uptimeSensor.value % 3600) / 60);
        document.getElementById('uptime-value').textContent = hours+'h '+minutes+'m';
      }
      
      // IP Adresse
      const ipSensor = textSensors.find(s=>s.id.includes('ip'));
      if(ipSensor){
        document.getElementById('ip-value').textContent = ipSensor.value;
      }
      
      // SSID
      const ssidSensor = textSensors.find(s=>s.id.includes('ssid'));
      if(ssidSensor){
        document.getElementById('ssid-value').textContent = ssidSensor.value;
      }
      
      // MAC
      const macSensor = textSensors.find(s=>s.id.includes('mac'));
      if(macSensor){
        document.getElementById('mac-value').textContent = macSensor.value;
      }
      
      // Update Zeit
      const now = new Date();
      document.getElementById('update-time').textContent = 
        'Letzte Aktualisierung: '+now.toLocaleTimeString();
        
    }catch(e){
      console.error('Fehler beim Laden der Daten:',e);
    }
  }
  
  // Initial laden
  updateDashboard();
  
  // Auto-Refresh starten
  setInterval(updateDashboard, REFRESH_INTERVAL);
})();

