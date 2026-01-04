#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <math.h>

// ==========================================
// CONFIGURATION (Update these values!)
// ==========================================
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASS"

// Firebase Settings
#define FIREBASE_HOST "YOUR_PROJECT_ID.firebaseio.com"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET"

#define DEVICE_ID "esp8266_01"

// ==========================================

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

const int micPin = A0;
const int sampleWindow = 300;
const float voltageRef = 3.3;
const float micSensitivity = 0.2512;
const float calibrationOffset = -12;

void setup()
{
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");

    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;

    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);

    if (Firebase.ready())
    {
        String statusPath = "/devices/" + String(DEVICE_ID) + "/status";
        Firebase.setString(firebaseData, statusPath, "online");
        Firebase.setString(firebaseData, statusPath + "/timestamp", String(millis()));
        Firebase.setString(firebaseData, statusPath, "offline", true); // onDisconnect
        Serial.println("Firebase Ready!");
    }
    else
    {
        Serial.println("Firebase Connection Error.");
    }
}

double measureNoise()
{
    unsigned long startMillis = millis();
    unsigned int signalMax = 0;
    unsigned int signalMin = 1023;

    while (millis() - startMillis < sampleWindow)
    {
        int sample = analogRead(micPin);
        if (sample < 1023)
        {
            if (sample > signalMax)
                signalMax = sample;
            if (sample < signalMin)
                signalMin = sample;
        }
    }

    unsigned int peakToPeak = signalMax - signalMin;
    double voltage = (peakToPeak * voltageRef) / 1023.0;

    if (voltage <= 0)
        return 0;
    return 20.0 * log10(voltage / (micSensitivity / 1000)) + calibrationOffset;
}

void loop()
{
    if (Firebase.ready())
    {
        double noiseLevel = measureNoise();
        String noisePath = "/node1/noise";

        if (Firebase.setDouble(firebaseData, noisePath, noiseLevel))
        {
            Serial.print("Noise Level (dB): ");
            Serial.println(noiseLevel);
        }
        else
        {
            Serial.println("Data Transmission Error.");
        }

        String statusPath = "/devices/" + String(DEVICE_ID) + "/status";
        if (Firebase.setString(firebaseData, statusPath, "online"))
        {
            Firebase.setString(firebaseData, statusPath + "/timestamp", String(millis()));
        }
    }
    delay(1000);
}