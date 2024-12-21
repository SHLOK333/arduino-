#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_wifi.h> // Include the esp_wifi.h header for station info functions

// Configuration for Access Point (Free Wi-Fi)
const char* apSSID = "FreeWiFi";

// Configuration for Internet Access (Station Mode)
const char* staSSID = "Shlok's";   // Replace with your router SSID
const char* staPassword = "12345678"; // Replace with your router password

// DNS and Web Server
DNSServer dnsServer;
WebServer server(80);
const byte DNS_PORT = 53;

// IP configuration for Access Point
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// List of signed-in users
struct User {
  String username;
  String macAddress;
};
std::vector<User> users;

// HTML Sign-In Page
String loginPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sign In</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background: #f4f4f4;
            padding: 50px;
        }
        form {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
            display: inline-block;
        }
        input[type="text"], input[type="submit"] {
            margin: 10px 0;
            padding: 10px;
            width: 80%;
        }
    </style>
</head>
<body>
    <h1>Welcome to Free Wi-Fi</h1>
    <p>Please sign in to access the internet</p>
    <form action="/connect" method="POST">
        <input type="text" name="username" placeholder="Enter your name" required>
        <input type="submit" value="Connect">
    </form>
</body>
</html>
)rawliteral";

// Function to handle root page (Sign-In Form)
void handleRoot() {
  server.send(200, "text/html", loginPage);
}

// Function to handle Sign-In
void handleConnect() {
  if (server.hasArg("username")) {
    String username = server.arg("username");

    // Get MAC address of the connected client
    wifi_sta_list_t stationList;
    esp_err_t err = esp_wifi_ap_get_sta_list(&stationList);
    String macAddress = "";

    if (err == ESP_OK && stationList.num > 0) {
      wifi_sta_info_t station = stationList.sta[0]; // Use the first connected station
      for (int i = 0; i < 6; i++) {
        macAddress += String(station.mac[i], HEX);
        if (i < 5) macAddress += ":";
      }
    }

    // Add the user to the list if not already present
    bool userExists = false;
    for (const auto& user : users) {
      if (user.macAddress == macAddress) {
        userExists = true;
        break;
      }
    }
    if (!userExists && macAddress != "") {
      users.push_back({username, macAddress});
    }

    // Response to the user
    server.send(200, "text/plain", "Welcome " + username + "! You are now connected to the internet.");
    Serial.println("User connected: " + username + " (" + macAddress + ")");
  } else {
    server.send(400, "text/plain", "Invalid request.");
  }
}

// Redirect all other requests to the Captive Portal
void redirectToCaptivePortal() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// Connect ESP32 to an external Wi-Fi for internet access
void connectToInternet() {
  WiFi.mode(WIFI_AP_STA); // Dual mode: AP + Station
  WiFi.begin(staSSID, staPassword);

  Serial.print("Connecting to internet...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to internet with IP: " + WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);

  // Start Access Point
  WiFi.softAP(apSSID);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  Serial.println("Access Point Started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS Server for Captive Portal
  dnsServer.start(DNS_PORT, "*", apIP);

  // Setup Web Server Routes
  server.on("/", handleRoot);
  server.on("/connect", HTTP_POST, handleConnect);
  server.onNotFound(redirectToCaptivePortal);

  // Start Web Server
  server.begin();
  Serial.println("Web Server Started");

  // Connect to Internet
  connectToInternet();
}

void loop() {
  dnsServer.processNextRequest(); // Handle DNS requests
  server.handleClient();          // Handle HTTP requests
}
