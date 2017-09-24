#include <ESP8266WiFi.h>
#include <ThingerESP8266.h>
#include <SPI.h>
#include "LedMatrix.h"
#include "DHT.h"

const char* thingerUser = "XXXX";
const char* deviceCode = "XXXX";
const char* deviceName = "XXXX";

#define ONBOARDLED D0

// LED Screen
#define NUMBER_OF_DEVICES 1
#define CS_PIN D4
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CS_PIN);

// WiFi access point details.
char *ssid = "XXXX";
char *password = "XXXX";

// DH11 pin connections
#define DHTPin D3     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

DHT dht(DHTPin, DHTTYPE);

// Thinger related
ThingerESP8266 *thing;

unsigned long timer;

void setup()
{
  Serial.begin(115200);
  timer = millis();

  connectToWiFi();
  initializeThingerAPI();
  Serial.println("Connected to Thinger");

  dht.begin();
  Serial.println("DH11 initialized");

  initializeLEDScreen();
}

void initializeLEDScreen() {
  ledMatrix.init();
  ledMatrix.setIntensity(4); // range is 0-15
  ledMatrix.setText("MAX7219 Demo"); // TODO: Will be cleaned up
}

// Connect to Thinger.io and initialize end points.
void initializeThingerAPI() {
  thing =  new ThingerESP8266(thingerUser, deviceName, deviceCode);

  (*thing)["led_glow"] << [](pson & in) {
    Serial.println("led_glow triggered");
  };
}


void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wifi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //blink(ONBOARDLED, 500, 500);
    delay(500);
  }
  Serial.println("");
  Serial.println("IP Address : ");
  Serial.println(WiFi.localIP());
}

void readTemperatureSensor() {

  float h = dht.readHumidity();
  Serial.println("Humidity ");
  Serial.println(h);

  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  Serial.println("Celsius ");
  Serial.println(t);

  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  Serial.println("Fahrenheit ");
  Serial.println(f);
}

void doActionEvery(int afterMilliSeconds) {
  if(millis() - timer > afterMilliSeconds) {
    Serial.print("Timing : ");
    Serial.println(timer);
    timer = millis();

    // Keep flashing to show alive state
    readTemperatureSensor();

  }

}

void loop()  {
  (*thing).handle();
  writeTextToDisplay();
  doActionEvery(5000);
  blink(ONBOARDLED, 500, 500);
}

void writeTextToDisplay() {
  ledMatrix.clear();
  ledMatrix.scrollTextLeft();
  ledMatrix.drawText();
  ledMatrix.commit();
  delay(200);
}

// Blinks an LED and again turns it off after 'x' seconds.
void blink(int pin_number, int on, int off)
{
  digitalWrite(pin_number, HIGH);
  delay(on);
  digitalWrite(pin_number, LOW);
  delay(off);
}
