#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define BUZZER_PIN 5 // D5 corresponds to GPIO23
const char* ssid = "Shlok's Galaxy S22";           // Replace with your WiFi SSID
const char* password = "12345678";   // Replace with your WiFi password

// Etherscan/Alchemy API and wallet details
String walletAddress = "0xa965a3478cEF1eEF42Ca96Ad60195540cB3cD8e3"; // Replace with your Ethereum wallet address
String apiKey = "ZT2WV1VPPWFCFQJNBBTMTDU31NCSUVSQN3";               // Replace with your API key
String apiUrl = "https://api-sepolia.etherscan.io/api?module=account&action=txlist&address=" + walletAddress + "&apikey=" + apiKey;


String lastTransactionHash = ""; // To track the last processed transaction

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT); // Initialize the buzzer pin
  WiFi.begin(ssid, password);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("API Response:");
      Serial.println(payload);

      // Parse the JSON response
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      JsonArray txList = doc["result"].as<JsonArray>();

      if (txList.size() > 0) {
        JsonObject latestTx = txList[0];
        String transactionHash = latestTx["hash"];
        String value = latestTx["value"];
        float ethValue = value.toFloat() / 1e18; // Convert Wei to ETH

        if (transactionHash != lastTransactionHash) {
          lastTransactionHash = transactionHash; // Update last transaction
          Serial.print("New Transaction Received: ");
          Serial.print(ethValue);
          Serial.println(" ETH");
          notifyTransaction(ethValue);
        }
      }
    } else {
      Serial.println("Error in HTTP request");
    }
    http.end();
  }
  delay(10000); // Check every 10 seconds
}

void notifyTransaction(float amount) {
  // Simple buzzer sound
  tone(BUZZER_PIN, 1000, 500); // 1kHz for 500ms
  delay(500);
  tone(BUZZER_PIN, 1500, 500); // 1.5kHz for 500ms
  delay(500);

  Serial.print("Notified: Received ");
  Serial.print(amount, 6);
  Serial.println(" ETH");
}
