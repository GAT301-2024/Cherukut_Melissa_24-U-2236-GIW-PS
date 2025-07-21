// Required Libraries for ESP32 Web Server
#include <WiFi.h> // For ESP32 Wi-Fi functionalities
#include <AsyncTCP.h> // Asynchronous TCP library, a dependency for ESPAsyncWebServer
#include <ESPAsyncWebServer.h> // Asynchronous Web Server library for ESP32

// --- Wi-Fi Configuration ---
// Define the SSID (network name) for your ESP32's Access Point
const char* ssid = "Cherukut";
// Define the password for your ESP32's Access Point. IMPORTANT: Change this to a strong, unique password!
const char* password = "eg+Cherukut"; // <--- CHANGE THIS PASSWORD!

// --- LED Pin Definitions (Based on Schematic) ---
// These GPIO pins control the BASE of the NPN transistors that switch the LEDs.
const int LED1_CTRL_PIN = 18;  // GPIO18 → Q1 base
const int LED2_CTRL_PIN = 19;  // GPIO19 → Q2 base
const int LED3_CTRL_PIN = 21;  // GPIO21 → Q3 base

// --- LDR Pin Definition (Based on Schematic) ---
const int LDR_PIN = 34; // Connected to GPIO 34 for analog reading

// --- Automatic Control Settings ---
// Threshold for determining night/day based on LDR reading.
// Values range from 0 (darkest) to 4095 (brightest) for a 12-bit ADC.
// You will likely need to CALIBRATE this value for your specific LDR and environment.
// Lower value = darker for "night" detection.
const int NIGHT_THRESHOLD = 800; // Example: Below 800 is considered night. ADJUST THIS!

// Delay between automatic light checks (in milliseconds)
const long AUTO_CHECK_INTERVAL = 10000; // Check every 10 seconds

// --- Web Server Object ---
// Create an instance of the AsyncWebServer on port 80 (standard HTTP port)
AsyncWebServer server(80);

// --- LED State Variables ---
// Boolean variables to keep track of the current state of each LED (true for ON, false for OFF)
bool led1State = false;
bool led2State = false;
bool led3State = false;

// --- Automatic Mode State Variable ---
bool autoModeEnabled = false;

// --- Timing Variable for Automatic Control ---
unsigned long lastAutoCheckMillis = 0;

// --- Helper function to set LED state (turns transistor ON/OFF) ---
void setLED(int pin, bool state) {
  // Since we are driving NPN transistors in common-emitter configuration:
  // HIGH on base turns transistor ON -> LED ON
  // LOW on base turns transistor OFF -> LED OFF
  digitalWrite(pin, state ? HIGH : LOW);
}

// --- HTML Content for the Web Dashboard ---
String getDashboardHtml() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Lighting Control | TEAM_A</title>
    <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --primary: #2E7D32;
            --primary-light: #4CAF50;
            --primary-lighter: #81C784;
            --secondary: #8BC34A;
            --accent: #CDDC39;
            --success: #388E3C;
            --danger: #F44336;
            --light: #F1F8E9;
            --dark: #1B5E20;
            --text: #2E7D32;
            --card-bg: #FFFFFF;
            --shadow: 0 4px 20px rgba(46, 125, 50, 0.15);
        }
        
        body {
            font-family: 'Poppins', sans-serif;
            background-color: #E8F5E9;
            margin: 0;
            padding: 20px;
            color: var(--text);
            min-height: 100vh;
        }
        
        .container {
            max-width: 1000px;
            margin: 0 auto;
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        
        .header-card {
            grid-column: 1 / -1;
            background-color: var(--card-bg);
            border-radius: 16px;
            padding: 20px;
            box-shadow: var(--shadow);
            text-align: center;
            border-left: 6px solid var(--primary);
        }
        
        .control-panel {
            background-color: var(--card-bg);
            border-radius: 16px;
            padding: 25px;
            box-shadow: var(--shadow);
            display: flex;
            flex-direction: column;
        }
        
        .status-panel {
            background-color: var(--card-bg);
            border-radius: 16px;
            padding: 25px;
            box-shadow: var(--shadow);
        }
        
        h1 {
            color: var(--primary);
            margin: 0;
            font-size: 1.8rem;
        }
        
        h2 {
            color: var(--primary-light);
            margin: 0 0 20px 0;
            font-size: 1.2rem;
            font-weight: 500;
        }
        
        h3 {
            color: var(--primary);
            margin-top: 0;
            border-bottom: 2px solid var(--light);
            padding-bottom: 10px;
            display: flex;
            align-items: center;
        }
        
        h3 i {
            margin-right: 10px;
            color: var(--primary-light);
        }
        
        .student-info {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin: 20px 0;
        }
        
        .info-item {
            background-color: var(--light);
            padding: 12px;
            border-radius: 10px;
            border-left: 3px solid var(--primary-light);
        }
        
        .info-label {
            font-size: 0.75rem;
            font-weight: 600;
            text-transform: uppercase;
            color: var(--primary);
            letter-spacing: 0.5px;
        }
        
        .info-value {
            font-size: 0.95rem;
            font-weight: 500;
            color: var(--dark);
            margin-top: 5px;
        }
        
        .wifi-status {
            background-color: var(--light);
            padding: 15px;
            border-radius: 10px;
            margin: 20px 0;
            display: flex;
            align-items: center;
        }
        
        .wifi-icon {
            font-size: 1.8rem;
            color: var(--primary);
            margin-right: 15px;
        }
        
        .wifi-details {
            flex: 1;
        }
        
        .wifi-name {
            font-weight: 600;
            color: var(--primary);
            margin-bottom: 5px;
        }
        
        .wifi-ip {
            font-size: 0.85rem;
            color: var(--text);
            opacity: 0.8;
        }
        
        .btn {
            background-color: var(--primary);
            color: white;
            border: none;
            padding: 14px;
            border-radius: 10px;
            font-size: 1rem;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s;
            display: flex;
            align-items: center;
            justify-content: center;
            margin: 8px 0;
            box-shadow: 0 4px 8px rgba(76, 175, 80, 0.3);
        }
        
        .btn:hover {
            background-color: var(--success);
            transform: translateY(-2px);
            box-shadow: 0 6px 12px rgba(76, 175, 80, 0.4);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .btn i {
            margin-right: 10px;
        }
        
        .btn.on {
            background-color: var(--success);
        }
        
        .led-grid {
            display: grid;
            grid-template-columns: 1fr;
            gap: 10px;
            margin: 15px 0;
        }
        
        .led-item {
            display: flex;
            align-items: center;
            justify-content: space-between;
            background-color: var(--light);
            padding: 15px;
            border-radius: 10px;
        }
        
        .led-info {
            display: flex;
            align-items: center;
        }
        
        .led-icon {
            font-size: 1.5rem;
            color: var(--primary);
            margin-right: 15px;
        }
        
        .led-name {
            font-weight: 500;
        }
        
        .status-indicator {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            border: 2px solid white;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        
        .status-indicator.on {
            background-color: var(--success);
            box-shadow: 0 0 15px rgba(76, 175, 80, 0.6);
        }
        
        .status-indicator.off {
            background-color: var(--danger);
        }
        
        .sensor-data {
            background-color: var(--light);
            padding: 20px;
            border-radius: 10px;
            margin-top: 20px;
        }
        
        .sensor-row {
            display: flex;
            justify-content: space-between;
            margin-bottom: 10px;
        }
        
        .sensor-label {
            font-weight: 500;
        }
        
        .sensor-value {
            font-weight: 600;
            color: var(--primary);
        }
        
        .auto-mode {
            margin-top: auto;
        }
        
        footer {
            grid-column: 1 / -1;
            text-align: center;
            margin-top: 20px;
            font-size: 0.85rem;
            color: var(--text);
            opacity: 0.8;
        }
        
        @media (max-width: 768px) {
            .container {
                grid-template-columns: 1fr;
            }
            
            .student-info {
                grid-template-columns: 1fr;
            }
        }
    </style>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css">
</head>
<body>
    <div class="container">
        <div class="header-card">
            <h1>Smart Lighting Control System</h1>
            <h2>TEAM_A Project | Cherukut Melissa (24/U/2236/GIW/PS)</h2>
        </div>
        
        <div class="control-panel">
            <h3><i class="fas fa-sliders-h"></i> Manual Controls</h3>
            
            <div class="led-grid">
                <div class="led-item">
                    <div class="led-info">
                        <i class="fas fa-lightbulb led-icon"></i>
                        <div class="led-name">LED 1</div>
                    </div>
                    <button id="led1Button" class="btn" onclick="toggleLED(1)">
                        <span id="led1ButtonText">Toggle</span>
                    </button>
                </div>
                
                <div class="led-item">
                    <div class="led-info">
                        <i class="fas fa-lightbulb led-icon"></i>
                        <div class="led-name">LED 2</div>
                    </div>
                    <button id="led2Button" class="btn" onclick="toggleLED(2)">
                        <span id="led2ButtonText">Toggle</span>
                    </button>
                </div>
                
                <div class="led-item">
                    <div class="led-info">
                        <i class="fas fa-lightbulb led-icon"></i>
                        <div class="led-name">LED 3</div>
                    </div>
                    <button id="led3Button" class="btn" onclick="toggleLED(3)">
                        <span id="led3ButtonText">Toggle</span>
                    </button>
                </div>
            </div>
            
            <div class="auto-mode">
                <h3><i class="fas fa-robot"></i> Automatic Mode</h3>
                <button id="autoModeButton" class="btn" onclick="toggleAutoMode()">
                    <i class="fas fa-magic"></i> <span id="autoModeButtonText">Enable Auto Mode</span>
                </button>
            </div>
        </div>
        
        <div class="status-panel">
            <h3><i class="fas fa-info-circle"></i> System Information</h3>
            
            <div class="student-info">
                <div class="info-item">
                    <div class="info-label">Student Name</div>
                    <div class="info-value">Cherukut Melissa</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Registration No</div>
                    <div class="info-value">24/U/2236/GIW/PS</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Project</div>
                    <div class="info-value">Smart Lighting</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Date</div>
                    <div class="info-value" id="currentDate">Loading...</div>
                </div>
            </div>
            
            <div class="wifi-status">
                <i class="fas fa-wifi wifi-icon"></i>
                <div class="wifi-details">
                    <div class="wifi-name" id="wifiSSID">Not connected</div>
                    <div class="wifi-ip" id="wifiStatus">IP: Loading...</div>
                </div>
            </div>
            
            <div class="sensor-data">
                <h3><i class="fas fa-sun"></i> Light Sensor Data</h3>
                
                <div class="sensor-row">
                    <span class="sensor-label">Current Light Level:</span>
                    <span class="sensor-value" id="ldrValue">---</span>
                </div>
                
                <div class="sensor-row">
                    <span class="sensor-label">Night Threshold:</span>
                    <span class="sensor-value" id="thresholdValue">800</span>
                </div>
                
                <div class="sensor-row">
                    <span class="sensor-label">Auto Mode Status:</span>
                    <span class="sensor-value" id="autoModeStatusText">Offline</span>
                </div>
            </div>
        </div>
        
        <footer>
            <p>© <span id="currentYear"></span> Smart Lighting Control System | TEAM_A Project</p>
        </footer>
    </div>

    <script>
        // Set current date and year
        const now = new Date();
        document.getElementById('currentDate').textContent = now.toLocaleDateString('en-US', { 
            weekday: 'long', 
            year: 'numeric', 
            month: 'long', 
            day: 'numeric' 
        });
        document.getElementById('currentYear').textContent = now.getFullYear();
        
        // Function to update WiFi information
        function updateWifiInfo(ssid, ip) {
            const wifiSSID = document.getElementById('wifiSSID');
            const wifiStatus = document.getElementById('wifiStatus');
            
            if (ssid) {
                wifiSSID.textContent = ssid;
                wifiStatus.textContent = `IP: ${ip || 'Not available'}`;
            } else {
                wifiSSID.textContent = "Not connected";
                wifiStatus.textContent = "Connect to ESP32 AP";
            }
        }
        
        // Function to send a request to the ESP32 to toggle an LED
        async function toggleLED(ledNum) {
            const button = document.getElementById(`led${ledNum}Button`);
            const buttonText = document.getElementById(`led${ledNum}ButtonText`);

            try {
                const response = await fetch(`/led${ledNum}/toggle`);
                const data = await response.text();
                updateUI(ledNum, data.includes("ON"));
            } catch (error) {
                console.error('Error toggling LED:', error);
                alert('Connection error. Please check your WiFi connection.');
            }
        }

        // Function to toggle Automatic Mode
        async function toggleAutoMode() {
            const button = document.getElementById('autoModeButton');
            const buttonText = document.getElementById('autoModeButtonText');
            const statusText = document.getElementById('autoModeStatusText');

            try {
                const response = await fetch('/automode/toggle');
                const data = await response.json();
                updateAutoModeUI(data.autoModeEnabled);
            } catch (error) {
                console.error('Error toggling Auto Mode:', error);
                alert('Connection error. Please check your WiFi connection.');
            }
        }

        // Function to fetch the current status from ESP32
        async function updateAllStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();

                updateUI(1, data.led1);
                updateUI(2, data.led2);
                updateUI(3, data.led3);
                updateAutoModeUI(data.autoModeEnabled);
                document.getElementById('ldrValue').textContent = data.ldrValue;
                
                if (data.wifiSSID) {
                    updateWifiInfo(data.wifiSSID, data.ipAddress);
                }

            } catch (error) {
                console.error('Error fetching system status:', error);
                updateWifiInfo(null);
            }
        }

        // Helper function to update LED UI
        function updateUI(ledNum, state) {
            const button = document.getElementById(`led${ledNum}Button`);
            const buttonText = document.getElementById(`led${ledNum}ButtonText`);

            if (state) {
                button.classList.add('on');
                buttonText.textContent = 'Turn OFF';
            } else {
                button.classList.remove('on');
                buttonText.textContent = 'Turn ON';
            }
        }

        // Helper function to update Auto Mode UI
        function updateAutoModeUI(state) {
            const button = document.getElementById('autoModeButton');
            const buttonText = document.getElementById('autoModeButtonText');
            const statusText = document.getElementById('autoModeStatusText');

            if (state) {
                button.classList.add('on');
                buttonText.textContent = 'Disable Auto Mode';
                statusText.textContent = 'Active';
            } else {
                button.classList.remove('on');
                buttonText.textContent = 'Enable Auto Mode';
                statusText.textContent = 'Inactive';
            }
        }

        // Initialize on load
        window.onload = function() {
            updateAllStatus();
            setInterval(updateAllStatus, 3000);
        };
    </script>
</body>
</html>
  )rawliteral";
  return html;
}


// --- Arduino Setup Function ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting ESP32 Intelligent Lighting System...");

  // Set LED control pins as OUTPUTs
  pinMode(LED1_CTRL_PIN, OUTPUT);
  pinMode(LED2_CTRL_PIN, OUTPUT);
  pinMode(LED3_CTRL_PIN, OUTPUT);
  
  // Set LDR pin as INPUT (implicitly done for analogRead, but good practice)
  pinMode(LDR_PIN, INPUT);

  // Initialize all LEDs to OFF state
  setLED(LED1_CTRL_PIN, LOW);
  setLED(LED2_CTRL_PIN, LOW);
  setLED(LED3_CTRL_PIN, LOW);
  led1State = false;
  led2State = false;
  led3State = false;

  // Start the ESP32 in Access Point (AP) mode
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point (AP) IP address: ");
  Serial.println(IP);
  Serial.print("Connect to Wi-Fi network: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.println("Then open a web browser and go to the IP address above.");

  // --- Web Server Route Handlers ---

  // Route for the root URL ("/") - serves the main HTML dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Client requested root URL '/'");
    request->send(200, "text/html", getDashboardHtml());
  });

  // Route to toggle LED 1
  server.on("/led1/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led1State = !led1State;
    setLED(LED1_CTRL_PIN, led1State);
    Serial.printf("LED 1 toggled to: %s\n", led1State ? "ON" : "OFF");
    request->send(200, "text/plain", led1State ? "LED1_ON" : "LED1_OFF");
  });

  // Route to toggle LED 2
  server.on("/led2/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led2State = !led2State;
    setLED(LED2_CTRL_PIN, led2State);
    Serial.printf("LED 2 toggled to: %s\n", led2State ? "ON" : "OFF");
    request->send(200, "text/plain", led2State ? "LED2_ON" : "LED2_OFF");
  });

  // Route to toggle LED 3
  server.on("/led3/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    led3State = !led3State;
    setLED(LED3_CTRL_PIN, led3State);
    Serial.printf("LED 3 toggled to: %s\n", led3State ? "ON" : "OFF");
    request->send(200, "text/plain", led3State ? "LED3_ON" : "LED3_OFF");
  });

  // Route to toggle Automatic Mode
  server.on("/automode/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    autoModeEnabled = !autoModeEnabled;
    Serial.printf("Automatic Mode toggled to: %s\n", autoModeEnabled ? "ENABLED" : "DISABLED");
    String jsonResponse = "{ \"autoModeEnabled\": " + String(autoModeEnabled ? "true" : "false") + " }";
    request->send(200, "application/json", jsonResponse);
  });

  // Route to get the current status of all LEDs, Auto Mode, and LDR value as JSON
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Serial.println("Client requested system status '/status'"); // Uncomment for more verbose logging
    int ldrValue = analogRead(LDR_PIN); // Read LDR value
    
    String jsonResponse = "{";
    jsonResponse += "\"led1\":" + String(led1State ? "true" : "false") + ",";
    jsonResponse += "\"led2\":" + String(led2State ? "true" : "false") + ",";
    jsonResponse += "\"led3\":" + String(led3State ? "true" : "false") + ",";
    jsonResponse += "\"autoModeEnabled\":" + String(autoModeEnabled ? "true" : "false") + ",";
    jsonResponse += "\"ldrValue\":" + String(ldrValue);
    jsonResponse += "}";
    request->send(200, "application/json", jsonResponse);
  });

  // Start the web server
  server.begin();
  Serial.println("Web server started.");
}

// --- Arduino Loop Function ---
// This function runs repeatedly after setup()
void loop() {
  // Check for automatic light control if enabled
  if (autoModeEnabled) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastAutoCheckMillis >= AUTO_CHECK_INTERVAL) {
      lastAutoCheckMillis = currentMillis;

      int ldrValue = analogRead(LDR_PIN);
      Serial.printf("LDR Value: %d, Threshold: %d\n", ldrValue, NIGHT_THRESHOLD);

      if (ldrValue < NIGHT_THRESHOLD) { // It's dark (LDR value is low)
        Serial.println("It's NIGHT - Turning ALL LEDs ON automatically.");
        if (!led1State) { led1State = true; setLED(LED1_CTRL_PIN, HIGH); }
        if (!led2State) { led2State = true; setLED(LED2_CTRL_PIN, HIGH); }
        if (!led3State) { led3State = true; setLED(LED3_CTRL_PIN, HIGH); }
      } else { // It's bright (LDR value is high)
        Serial.println("It's DAY - Turning ALL LEDs OFF automatically.");
        if (led1State) { led1State = false; setLED(LED1_CTRL_PIN, LOW); }
        if (led2State) { led2State = false; setLED(LED2_CTRL_PIN, LOW); }
        if (led3State) { led3State = false; setLED(LED3_CTRL_PIN, LOW); }
      }
    }
  }
}
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
