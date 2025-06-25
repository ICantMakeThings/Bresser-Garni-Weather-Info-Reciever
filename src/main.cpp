#include <Arduino.h> // Arduino, what this is based off!
#include "WeatherSensorCfg.h" // weather stuff
#include "WeatherSensor.h" // weather stuff
#include "InitBoard.h" // weather stuff
#include <ESP8266WiFi.h> // The base, WIFI!
#include <ESP8266WebServer.h> // The actual server
#include <WiFiManager.h> // To get that AP config
#include <ESP8266mDNS.h> // To get weather.lan

ESP8266WebServer server(80); // Server on port 80
WeatherSensor ws;
String latestSensorData = "No data yet.\n";

void handlePage() { // Loads the UI, with html and js
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
    <head>
      <meta charset="utf-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1" />
      <title>Bresser Weather Dashboard</title>
      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
      <style>
      .status-bar {
        position: absolute;
        top: 20px;
        right: 20px;
        background: #1e1e1e;
        padding: 8px 16px;
        border-radius: 12px;
        font-size: 0.95rem;
        box-shadow: 0 0 10px rgba(0, 188, 212, 0.25);
       color: #00bcd4;
       z-index: 9999;
      }
      .status-bar.low-battery #batteryDisplay {
        color: #ff5252;
      }
      .status-bar.weak-signal #rssiDisplay {
        color: #ffc107;
      }
        /* Reset & base */
        * {
          box-sizing: border-box;
          margin: 0;
          padding: 0;
          font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        }
        body {
          background: #121212;
          color: #e0e0e0;
          padding: 20px;
          display: flex;
          flex-direction: column;
          align-items: center;
          min-height: 100vh;
          -webkit-font-smoothing: antialiased;
          -moz-osx-font-smoothing: grayscale;
        }
        h2, h3 {
          font-weight: 600;
          margin-bottom: 12px;
          color: #00bcd4;
          letter-spacing: 1px;
        }
        /* Container for all */
        .dashboard {
          max-width: 900px;
          width: 100%;
          display: flex;
          flex-direction: column;
          gap: 30px;
        }
        /* Live Data text block */
        #data {
          background: #1e1e1e;
          border-radius: 12px;
          padding: 16px 24px;
          font-family: monospace;
          font-size: 1.1rem;
          line-height: 1.5;
          box-shadow: 0 4px 12px rgba(0, 188, 212, 0.25);
          white-space: pre-line;
          user-select: none;
        }
        /* Flex containers for gauges and wind */
        .gauges {
          display: flex;
          justify-content: center;
          gap: 40px;
        }
        .gauge-container {
          background: #1e1e1e;
          border-radius: 14px;
          padding: 20px;
          box-shadow: 0 4px 20px rgba(0, 188, 212, 0.3);
          text-align: center;
          width: 170px;
        }
        .gauge-container canvas {
          display: block;
          margin: 0 auto;
          background: #121212;
          border-radius: 50%;
          box-shadow: inset 0 0 12px #00bcd4;
        }
        /* Wind section */
        .wind-section {
          background: #1e1e1e;
          border-radius: 14px;
          padding: 24px 30px;
          box-shadow: 0 4px 24px rgba(0, 188, 212, 0.3);
          display: flex;
          flex-direction: column;
          align-items: center;
          width: 260px;
          margin: 0 auto;
        }
        #windCompass {
          background: #121212;
          border-radius: 50%;
          box-shadow: inset 0 0 15px #00bcd4;
          margin-bottom: 15px;
        }
        #windSpeed {
          font-size: 1.3rem;
          font-weight: 700;
          color: #00bcd4;
          user-select: none;
        }
        /* Chart container */
        .chart-container {
          background: #1e1e1e;
          border-radius: 14px;
          padding: 20px;
          box-shadow: 0 4px 24px rgba(0, 188, 212, 0.3);
        }
        canvas {
          display: block;
          max-width: 100%;
          height: auto;
          user-select: none;
        }
        /* Responsive */
        @media (max-width: 720px) {
          .gauges {
            flex-direction: column;
            align-items: center;
          }
          .gauge-container {
            width: 90vw;
            max-width: 300px;
          }
          .wind-section {
            width: 90vw;
          }
        }
      </style>
    </head>
    <body>
    <div class="status-bar">
      <span id="rssiDisplay">RSSI: --</span> |
      <span id="batteryDisplay">Battery: --</span>
    </div>
      <div class="dashboard">
        <h2>Live Weather Data</h2>
        <pre id="data">Loading...</pre>

        <h3>Light &amp; UV Index</h3>
        <div class="gauges">
          <div class="gauge-container">
            <canvas id="lightGauge" width="150" height="150"></canvas>
          </div>
          <div class="gauge-container">
            <canvas id="uvGauge" width="150" height="150"></canvas>
          </div>
        </div>

        <h3>Wind Direction &amp; Speed</h3>
        <div class="wind-section">
          <canvas id="windCompass" width="200" height="200"></canvas>
          <div id="windSpeed">-- m/s</div>
        </div>

        <div class="chart-container">
          <canvas id="weatherChart" width="400" height="200"></canvas>
        </div>
      </div>

      <script>
      let lastUpdateTime = Date.now();
        // Chart.js setup
        const ctx = document.getElementById('weatherChart').getContext('2d');
        const chart = new Chart(ctx, {
          type: 'line',
          data: {
            labels: [],
            datasets: [
              {
                label: 'Temperature (°C)',
                borderColor: '#ef5350',
                backgroundColor: 'rgba(239,83,80,0.3)',
                fill: true,
                data: [],
                yAxisID: 'y1',
                tension: 0.3,
                pointRadius: 3,
              },
              {
                label: 'Humidity (%)',
                borderColor: '#42a5f5',
                backgroundColor: 'rgba(66,165,245,0.3)',
                fill: true,
                data: [],
                yAxisID: 'y2',
                tension: 0.3,
                pointRadius: 3,
              }
            ]
          },
          options: {
            responsive: true,
            interaction: {
              mode: 'index',
              intersect: false,
            },
            stacked: false,
            scales: {
              y1: {
                type: 'linear',
                position: 'left',
                min: 0,
                max: 50,
                grid: { color: 'rgba(255,255,255,0.1)' },
                ticks: { color: '#ef5350' }
              },
              y2: {
                type: 'linear',
                position: 'right',
                min: 0,
                max: 100,
                grid: { drawOnChartArea: false },
                ticks: { color: '#42a5f5' }
              },
              x: {
                ticks: { color: '#bbb' },
                grid: { color: 'rgba(255,255,255,0.1)' }
              }
            },
            plugins: {
              legend: {
                labels: { color: '#bbb', font: { weight: '600' } }
              },
              tooltip: {
                enabled: true,
                backgroundColor: '#222',
                titleColor: '#00bcd4',
                bodyColor: '#fff'
              }
            }
          }
        });

        // Elements for gauges and wind
        const lightCanvas = document.getElementById('lightGauge');
        const uvCanvas = document.getElementById('uvGauge');
        const windCanvas = document.getElementById('windCompass');
        const windSpeedDiv = document.getElementById('windSpeed');

        // Sleek gauge drawing
        function drawGauge(canvas, value, max, label, unit) {
          const ctx = canvas.getContext('2d');
          const w = canvas.width;
          const h = canvas.height;
          const centerX = w / 2;
          const centerY = h / 2;
          const radius = Math.min(w, h) / 2 - 15;

          ctx.clearRect(0, 0, w, h);

          // Background ring
          ctx.beginPath();
          ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
          ctx.strokeStyle = '#333';
          ctx.lineWidth = 12;
          ctx.stroke();

          // Progress arc
          const angle = (value / max) * 2 * Math.PI;
          ctx.beginPath();
          ctx.strokeStyle = '#00bcd4';
          ctx.lineWidth = 12;
          ctx.lineCap = 'round';
          ctx.arc(centerX, centerY, radius, -Math.PI / 2, -Math.PI / 2 + angle);
          ctx.stroke();

          // Center text (value)
          ctx.fillStyle = '#e0e0e0';
          ctx.font = '28px "Segoe UI", Tahoma, Geneva, Verdana, sans-serif';
          ctx.textAlign = 'center';
          ctx.textBaseline = 'middle';
          ctx.fillText(value.toFixed(1) + (unit ? ' ' + unit : ''), centerX, centerY);

          // Label below value
          ctx.fillStyle = '#00bcd4';
          ctx.font = '16px "Segoe UI", Tahoma, Geneva, Verdana, sans-serif';
          ctx.fillText(label, centerX, centerY + radius / 1.7);
        }

        // Sleek wind compass with arrow
        function drawWindCompass(canvas, windDir) {
          const ctx = canvas.getContext('2d');
          const w = canvas.width;
          const h = canvas.height;
          const centerX = w / 2;
          const centerY = h / 2;
          const radius = Math.min(w, h) / 2 - 25;

          ctx.clearRect(0, 0, w, h);

          // Outer circle
          ctx.beginPath();
          ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
          ctx.strokeStyle = '#333';
          ctx.lineWidth = 6;
          ctx.shadowColor = 'rgba(0, 188, 212, 0.6)';
          ctx.shadowBlur = 10;
          ctx.stroke();
          ctx.shadowBlur = 0;

          // Cardinal points
          ctx.fillStyle = '#00bcd4';
          ctx.font = '18px "Segoe UI", Tahoma, Geneva, Verdana, sans-serif';
          ctx.textAlign = 'center';
          ctx.textBaseline = 'middle';
          const directions = ['N', 'E', 'S', 'W'];
          directions.forEach((dir, i) => {
            const angle = (Math.PI / 2) * i - Math.PI / 2;
            const x = centerX + Math.cos(angle) * (radius - 25);
            const y = centerY + Math.sin(angle) * (radius - 25);
            ctx.fillText(dir, x, y);
          });

          // Wind arrow
          if (!isNaN(windDir)) {
            const angleRad = (windDir - 90) * (Math.PI / 180);
            const arrowLength = radius - 40;

            ctx.save();
            ctx.translate(centerX, centerY);
            ctx.rotate(angleRad);

            // Arrow shaft
            ctx.beginPath();
            ctx.moveTo(0, 0);
            ctx.lineTo(arrowLength, 0);
            ctx.strokeStyle = '#00bcd4';
            ctx.lineWidth = 6;
            ctx.shadowColor = 'rgba(0, 188, 212, 0.8)';
            ctx.shadowBlur = 8;
            ctx.stroke();

            // Arrowhead
            ctx.beginPath();
            ctx.moveTo(arrowLength, 0);
            ctx.lineTo(arrowLength - 18, 10);
            ctx.lineTo(arrowLength - 18, -10);
            ctx.closePath();
            ctx.fillStyle = '#00bcd4';
            ctx.shadowBlur = 8;
            ctx.fill();

            ctx.restore();
          }
        }
       setInterval(() => {
          const now = Date.now();
          const rssiEl = document.getElementById("rssiDisplay");
          const statusBar = document.querySelector(".status-bar");

        if (now - lastUpdateTime > 10000) {
            rssiEl.innerText = "Server Offline";
            statusBar.classList.add("weak-signal");
         }
        }, 1000);

        // Fetch data and update UI
        async function fetchData() {
     try {
      const res = await fetch("/data");
       if (!res.ok) throw new Error("Network error");
        const json = await res.json();

       const time = new Date().toLocaleTimeString();

       document.getElementById("data").innerText =
          "Temperature: " + json.temperature + " °C\n" +
          "Humidity: " + json.humidity + " %\n" +
          "Wind Gust: " + json.windGust + " m/s\n" +
          "Wind Avg: " + json.windAvg + " m/s\n" +
          "Wind Dir: " + json.windDir + " °\n" +
          "Rain: " + json.rain + " mm\n" +
          "UV Index: " + json.uv + "\n" +
          "Light: " + json.light + " klx\n";

    // Battery Status
    const batteryOk = json.batteryOk;
    const batteryEl = document.getElementById("batteryDisplay");
    const statusBar = document.querySelector(".status-bar");

    if (batteryOk === false) {
      batteryEl.innerText = "🪫";
      statusBar.classList.add("low-battery");
    } else {
      batteryEl.innerText = "🔋";
      statusBar.classList.remove("low-battery");
    }

    // RSSI Display
    const rssi = json.rssi;
    const rssiEl = document.getElementById("rssiDisplay");

    if (typeof rssi === "number") {
      rssiEl.innerText = `RSSI: ${rssi} dBm`;
      if (rssi < -85) {
        statusBar.classList.add("weak-signal");
      } else {
        statusBar.classList.remove("weak-signal");
      }
        lastUpdateTime = Date.now();
    }


            // Update chart
            if (!isNaN(json.temperature) && !isNaN(json.humidity)) {
              chart.data.labels.push(time);
              chart.data.datasets[0].data.push(json.temperature);
              chart.data.datasets[1].data.push(json.humidity);

              if (chart.data.labels.length > 20) {
                chart.data.labels.shift();
                chart.data.datasets[0].data.shift();
                chart.data.datasets[1].data.shift();
              }
              chart.update();
            }

            // Update gauges
            drawGauge(lightCanvas, isNaN(json.light) ? 0 : json.light, 100, "Light", "klx");
            drawGauge(uvCanvas, isNaN(json.uv) ? 0 : json.uv, 11, "UV Index", "");

            // Update wind compass & speed
            drawWindCompass(windCanvas, isNaN(json.windDir) ? 0 : json.windDir);
            windSpeedDiv.innerText = "Wind Avg Speed: " + (isNaN(json.windAvg) ? "--" : json.windAvg.toFixed(1)) + " m/s";

          } catch (e) {
            console.error("Fetch failed", e);
          }
        }

        setInterval(fetchData, 2000);
        window.onload = fetchData;
      </script>
      
      <footer style="text-align:center; margin-top: 40px; color: #555; font-size: 0.9rem; font-style: italic;">
        Made by ICantMakeThings ;> https://icmt.cc
      </footer>
    </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void setupMDNS() {// Set local hostname weather.lan only on supported home networks!
  String base = "weather";
  int suffix = 0;
  bool started = false;

  while (!started && suffix < 10) {
    String hostname = base + (suffix == 0 ? "" : String(suffix));
    WiFi.hostname(hostname);

    started = MDNS.begin(hostname.c_str());

    if (started) {
      Serial.println("MDNS started successfully.");
      Serial.print("Access your device at: http://");
      Serial.print(hostname);
      Serial.println(".lan (OR .local)");
    } else {
      suffix++;
      delay(100);
    }
  }

  if (!started) {
    Serial.println("Failed to start MDNS after 10 attempts.");
  }
}


// Send sensor data as JSON:
void handleData() {
  if (latestSensorData.startsWith("{")) {
    server.send(200, "application/json", latestSensorData);
  } else {
    server.send(200, "application/json", "{}");
  }
}

// Setup the WiFi, server, and everything, Loading.
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Serial.printf("Starting execution...\n");
  initBoard();
  ws.begin();

  WiFiManager wm;

  if (!wm.autoConnect("WeatherSetupAP", "ICantMakeThings")) {
    Serial.println("Failed to connect, restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  setupMDNS();
  server.on("/", handlePage);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

// If command resetwifi gets sent, it wipes everything.
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.equalsIgnoreCase("resetwifi")) {
      Serial.println("Resetting WiFi settings...");
      WiFi.disconnect(true);  
      delay(500);
      ESP.eraseConfig();       
      delay(500);
      ESP.restart();          
    }
  }

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck >= 1000) {
    lastCheck = millis();

    ws.clearSlots();
    int decode_status = ws.getMessage();

    if (decode_status == DECODE_OK) {
      int i = 0;

      float temp     = ws.sensor[i].w.temp_ok     ? ws.sensor[i].w.temp_c : NAN;
      float hum      = ws.sensor[i].w.humidity_ok ? ws.sensor[i].w.humidity : NAN;
      float windGust = ws.sensor[i].w.wind_ok     ? ws.sensor[i].w.wind_gust_meter_sec : NAN;
      float windAvg  = ws.sensor[i].w.wind_ok     ? ws.sensor[i].w.wind_avg_meter_sec : NAN;
      float windDir  = ws.sensor[i].w.wind_ok     ? ws.sensor[i].w.wind_direction_deg : NAN;
      float rain     = ws.sensor[i].w.rain_ok     ? ws.sensor[i].w.rain_mm : NAN;
      float uv       = ws.sensor[i].w.uv_ok       ? ws.sensor[i].w.uv : NAN;
      float light    = ws.sensor[i].w.light_ok    ? ws.sensor[i].w.light_klx : NAN;
      bool batteryOk = ws.sensor[i].battery_ok;
      int rssi       = ws.sensor[i].rssi;


      latestSensorData = "{";
      latestSensorData += "\"temperature\":" + (isnan(temp) ? "null" : String(temp, 1)) + ",";
      latestSensorData += "\"humidity\":" + (isnan(hum) ? "null" : String(hum, 1)) + ",";
      latestSensorData += "\"windGust\":" + (isnan(windGust) ? "null" : String(windGust, 1)) + ",";
      latestSensorData += "\"windAvg\":" + (isnan(windAvg) ? "null" : String(windAvg, 1)) + ",";
      latestSensorData += "\"windDir\":" + (isnan(windDir) ? "null" : String(windDir, 1)) + ",";
      latestSensorData += "\"rain\":" + (isnan(rain) ? "null" : String(rain, 1)) + ",";
      latestSensorData += "\"uv\":" + (isnan(uv) ? "null" : String(uv, 1)) + ",";
      latestSensorData += "\"light\":" + (isnan(light) ? "null" : String(light, 1)) + ",";
      latestSensorData += "\"batteryOk\":" + String(batteryOk ? "true" : "false") + ",";
      latestSensorData += "\"rssi\":" + String(rssi);
      latestSensorData += "}";


      
      Serial.println("Updated sensor data: " + latestSensorData);
    }
  }
}
