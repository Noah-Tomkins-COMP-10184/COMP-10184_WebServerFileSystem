/**
 * I, Noah Tomkins, 0000790079 certify that this material is my original work.
 * No other person's work has been used without due acknowledgement.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define VERSION     "1.0"
#define TEMP        D7

const char *ssid = "";
const char *password = "";

ESP8266WebServer webServer(80);

OneWire oneWire(TEMP);
DallasTemperature DS18B20(&oneWire);
DeviceAddress thermometer;
bool connected = false;

unsigned long tempReadFrequency = 1000; // every 1 seconds
unsigned long lastTempTime = 0;
float temp = 0;

/*** Get Data Strings ***/
String getTempString() {
    if (connected)
        return String(temp);
    else
        return "";
}

String getAddressString(DeviceAddress deviceAddress)
{ 
    String addr = "";

    for (uint8_t i = 0; i < 8; i++)
    {
        addr += "0x";
        if (deviceAddress[i] < 0x10) Serial.print("0");
        addr += String(deviceAddress[i], HEX);
    }
   
    return addr;
}

/*** Update Sensors ***/
void updateSensors() {
    if (millis() - lastTempTime >= tempReadFrequency) {
        DS18B20.getAddress(thermometer, 0);

        if (DS18B20.isConnected(thermometer)) {
            connected = true;
            DS18B20.requestTemperatures();
            temp = DS18B20.getTempC(thermometer);
        } else {
            connected = false;
        }

        lastTempTime = millis();
    } 
}

/*** Web Handlers ***/
void handleVersion()
{
    String version = "Version: " + String(VERSION) + "\n";
    webServer.send(200, "text/plain", version);
}

void handleIndex()
{
    File index = LittleFS.open("/index.html", "r");

    if (index)
    {
        if (webServer.streamFile(index, "text/html") != index.size())
            Serial.println("Failed to load index.html");

        index.close();
    }
    else
    {
        webServer.send(200, "text/plain", "Failed to load index.html");
    }
}

void getDataJSON() {
    StaticJsonDocument<100> doc;
    doc["temp"] = getTempString();
    doc["sensor_id"] = getAddressString(thermometer);

    String data;
    serializeJson(doc, data);

    webServer.send(200, "application/json", data);
}

/*** Main Program ***/
void setup()
{
    // Setup Serial
    Serial.begin(115200);
    Serial.println("\nNoah's Web Server");

    // Setup Filesystem
    LittleFS.begin();
    Serial.println("File System Initialized");

    // Setup Thermometer
    DS18B20.begin();

    // Setup WiFi
    Serial.printf("\nConnecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }

    // Setup WebServer
    webServer.on("/version", handleVersion);
    webServer.on("/", handleIndex);
    webServer.on("/getData", getDataJSON);
    webServer.begin();

    Serial.printf("\nWeb Server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());
}

void loop()
{
    webServer.handleClient();
    updateSensors();
}