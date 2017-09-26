#include <ESP8266WiFi.h>
#include <ThingerESP8266.h>
#include <SPI.h>
#include "LedMatrix.h"
#include "DHT.h"

const char* thingerUser = "xxxxx"; // FIXME: Replace with Real authentication
const char* deviceCode = "xxxxx"; // FIXME: Replace with Real authentication
const char* deviceName = "xxxxx"; // FIXME: Replace with Real authentication

#define ONBOARDLED D0

// LED Screen
#define NUMBER_OF_DEVICES 2
#define CS_PIN D4
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

// WiFi access point details.
char *ssid = "xxxxx"; // FIXME: Replace with Real authentication
char *password = "xxxxx"; // FIXME: Replace with Real authentication

// DH11 pin connections
#define DHTPin D3     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

DHT dht(DHTPin, DHTTYPE);

// Thinger related
ThingerESP8266 *thing;

// Supports doing action every 'x' seconds
unsigned long timer;

const char* host = "XXXX.service-now.com"; // FIXME: Replace with Real ServiceNow instance
const int httpsPort = 443;

String lastReadTemperature = String("");

#define FAN D1


void setup()
{
  Serial.begin(115200);
  timer = millis();
  pinMode(FAN, OUTPUT);


  connectToWiFi();
  initializeThingerAPI();
  initializeTemperatureSensor();
  initializeLEDScreen();
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

// Connect to Thinger.io and initialize end points.
void initializeThingerAPI() {
  thing =  new ThingerESP8266(thingerUser, deviceName, deviceCode);

  // TODO: Remove these dummy methods and add as per the requirement. 
  (*thing)["led_glow"] << [](pson & in) {
    Serial.println("led_glow triggered");
  };

  Serial.println("Connected to Thinger");

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

// Read the temperature sensor and return the value as a String
String readTemperatureSensor() {
  // Read temperature as Celsius (the default)

  int t = dht.readTemperature();

  if (isnan(t) || t > 50) {
    Serial.println("Invalid Temperature or NAN detected, will not utilize this read, returns the last reading. ");
    return lastReadTemperature;
  }

  Serial.println("Celsius ");
  lastReadTemperature = String(t);
  Serial.println(lastReadTemperature);
  ledMatrix.setText(lastReadTemperature);

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
    blink(ONBOARDLED, 500, 500);
    uploadTemperature("DEVICE", temperatureReading);
    digitalWrite(FAN, !digitalRead(FAN));
    return true;
  }
  return false;
}

void loop()  {

  // If the action time is not elapsed, then execute handle(), else execute Thinger.
  if (!doActionEvery(10 * 1000)) {
    // (*thing).handle(); // Temporarily disabling this as its failing when making REST POST calls.
  }

  writeTextToDisplay();

}

void writeTextToDisplay() {
  ledMatrix.clear();
  ledMatrix.scrollTextLeft();
  ledMatrix.drawText();
  ledMatrix.commit();
  delay(200);
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


  String url = "/api/now/table/XXXXXX"; // FIXME: Replace with the Table to write to.
  String jsonContent = "{'device_identifier': '" + device_id + "','temperature':'" + temperatureAsString + "'}";

  Serial.println("Sending Temperature to ServiceNow Instance ");
  Serial.println(jsonContent);

  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Content-Type: application/json\r\n" +
               "Accept: application/json\r\n" +
               "Authorization: Basic XXXXXXXXr\n" + // FIXME: Replace with Real authentication (username:password Base64)
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

}

// Blinks an LED and again turns it off after 'x' seconds.
void blink(int pin_number, int on, int off)
{
  digitalWrite(pin_number, HIGH);
  delay(on);
  digitalWrite(pin_number, LOW);
  delay(off);
}
