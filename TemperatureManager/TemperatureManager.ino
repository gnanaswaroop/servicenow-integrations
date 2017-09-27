#include <ESP8266WiFi.h>
#include <ThingerESP8266.h>
#include <SPI.h>
#include "LedMatrix.h"
#include "DHT.h"

const char* thingerUser = "XXXXX";
const char* deviceCode = "XXXXX";
const char* deviceName = "XXXXX";

#define ONBOARDLED D0

// LED Screen
#define NUMBER_OF_DEVICES 2
#define CS_PIN D4
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

// WiFi access point details.
char *ssid = "XXXXX";
char *password = "XXXXX";

// DH11 pin connections
#define DHTPin D3     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPin, DHTTYPE);

// Supports doing action every 'x' seconds
unsigned long timer;

const char* host = "XXXXX.service-now.com";
const int httpsPort = 443;
const String auth = "XXXXX"; // Base64

String lastReadTemperature = String("");

#define FAN D1

int thresholdTemperature = 28; // In celsius

const int NORMAL_INTERVAL = 10 * 1000;
const int EMERGENCY_INTERVAL = 5 * 1000;

int selectedInterval = NORMAL_INTERVAL;

const int MAX_VALID_ROOM_TEMPERATURE = 50;

const String DEVICE_ID = "DEVICE"; // This can be retrieved as a MAC Address. Hard-coded for simplicity

void setup()
{
  Serial.begin(115200);
  timer = millis();
  pinMode(FAN, OUTPUT);

  connectToWiFi();
  turnFANOff();

  initializeTemperatureSensor();
  initializeLEDScreen();
}

void loop()  {
  doActionEvery(selectedInterval);
  writeTextToDisplay();
}

void writeTextToDisplay() {
  ledMatrix.clear();
  ledMatrix.scrollTextLeft();
  ledMatrix.drawText();
  ledMatrix.commit();
  delay(200);
}

void initializeLEDScreen() {
  ledMatrix.init();
  ledMatrix.setIntensity(4); // range is 0-15
  ledMatrix.setText("Temperature Manager");
  Serial.println("LED Screen Initialized");
}

void initializeTemperatureSensor() {
  dht.begin();
  Serial.println("DH11 initialized");
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wifi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    blink(ONBOARDLED, 500, 500);
    delay(500);
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP Address : ");
  Serial.println(WiFi.localIP());
}

// Write the Low bit to turn on the fan (via relay)
void turnFanON() {
  Serial.println("FAN turning ON");
  digitalWrite(FAN, LOW);
  uploadFanState(DEVICE_ID, true);
  // selectedInterval = EMERGENCY_INTERVAL;
}

// Write the High bit to turn on the fan (via relay)
void turnFANOff() {
  Serial.println("FAN turning OFF");
  digitalWrite(FAN, HIGH);
  uploadFanState(DEVICE_ID, false);
  // selectedInterval = NORMAL_INTERVAL;
}

boolean isFANOn() {
  bool fanStatus = digitalRead(FAN);
  Serial.println();

  Serial.print("Is Fan on right now - ");
  Serial.println(!fanStatus);

  return !fanStatus;
}

// Read the temperature sensor and return the value as a String
String readTemperatureSensor() {
  // Read temperature as Celsius (the default)
  int currentTemperature = dht.readTemperature();

  // Return last read temperature if invalid temperature is detected.
  if (isnan(currentTemperature) || currentTemperature > MAX_VALID_ROOM_TEMPERATURE) {
    Serial.print("Invalid Temperature or NAN detected, will not utilize this read, returns the last reading. ");
    Serial.println(currentTemperature);
    return lastReadTemperature;
  }

  lastReadTemperature = String(currentTemperature);

  Serial.print("Temperature detected in celsius ");
  Serial.println(currentTemperature);

  // If the threshold is crossed, keep the fan on.
  if (currentTemperature >= thresholdTemperature && !isFANOn()) {
    Serial.println("Temperature exceeds threshold temperature ");
    turnFanON();
  } else if (currentTemperature < thresholdTemperature && isFANOn()) { // If the temperature falls, turn off the FAN if its already ON.
    Serial.println("Temperature returns back to less than threshold temperature ");
    turnFANOff();
  }

  return lastReadTemperature;
}

// Returns true if an action was taken.
bool doActionEvery(int afterMilliSeconds) {

  // Only execute if the time has elapsed the parameter milliseconds
  if (millis() - timer > afterMilliSeconds) {
    Serial.print("Timing : ");
    Serial.println(timer);
    timer = millis();

    // Keep flashing to show alive state
    String temperatureReading = readTemperatureSensor();

    // Set to Display.
    ledMatrix.setText(temperatureReading);

    blink(ONBOARDLED, 500, 500);
    uploadTemperature("DEVICE", temperatureReading);
    return true;
  }
  // return false;
}


// register the device on servicenow instance with its MAC address.
void uploadTemperature(String device_id, String temperatureAsString) {

  Serial.println("Starting to upload temperature to a ServiceNow instance ");

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  if (!client.connect(host, httpsPort)) {
    Serial.println("Failed to connect to a ServiceNow instance");

    return;
  }

  Serial.println("Connected to a ServiceNow instance ");


  String url = "/api/now/table/XXXXX"; // Change the table name
  String jsonContent = "{'device_identifier': '" + device_id + "','temperature':'" + temperatureAsString + "'}";

  Serial.println("Sending Temperature to ServiceNow Instance ");
  Serial.println(jsonContent);

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Content-Type: application/json\r\n" +
               "Accept: application/json\r\n" +
               "Authorization: Basic " + auth + "\r\n" +
               "Content-Length: " + jsonContent.length() + "\r\n" +
               "Connection: close\r\n\r\n" +
               jsonContent);

  // bypass HTTP headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //Serial.println( "Header: " + line );
    if (line == "\r") {
      break;
    }
  }
  //  read body length
  int bodyLength = 0;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //Serial.println( "Body length: " + line );
    bodyLength = line.toInt();
    break;
  }

  Serial.println("Completed sending Temperature to ServiceNow Instance ");


}

// register the device on servicenow instance with its MAC address.
void uploadFanState(String device_id, boolean fanState) {

  Serial.println("Starting to upload fan state to a ServiceNow instance ");

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  if (!client.connect(host, httpsPort)) {
    Serial.println("Failed to connect to a ServiceNow instance");

    return;
  }

  Serial.println("Connected to a ServiceNow instance ");


  String url = "/api/now/table/XXXXX"; // Change the table name
  String jsonContent = "{'u_device_identifier': '" + device_id + "','u_fan_state':'" + (fanState == true ? "ON" : "OFF") + "'}";

  Serial.println("Sending Fan State to ServiceNow Instance ");
  Serial.println(jsonContent);

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Content-Type: application/json\r\n" +
               "Accept: application/json\r\n" +
               "Authorization: Basic " + auth + "\r\n" +
               "Content-Length: " + jsonContent.length() + "\r\n" +
               "Connection: close\r\n\r\n" +
               jsonContent);

  // bypass HTTP headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //Serial.println( "Header: " + line );
    if (line == "\r") {
      break;
    }
  }
  //  read body length
  int bodyLength = 0;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //Serial.println("Body length: " + line );
    bodyLength = line.toInt();
    break;
  }

  Serial.println("Completed sending Fan State to ServiceNow Instance ");
}

// Blinks an LED and again turns it off after 'x' seconds.
void blink(int pin_number, int on, int off)
{
  digitalWrite(pin_number, HIGH);
  delay(on);
  digitalWrite(pin_number, LOW);
  delay(off);
}
