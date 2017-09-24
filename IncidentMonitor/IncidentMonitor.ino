/*
    Displays different color LEDs based on an Incident status on ServiceNow
*/

#include <ESP8266WiFi.h>
#include <ThingerESP8266.h>

// Define the data ports each of the LED is connected to.
#define ONBOARDLED D0
#define RED_LED D2
#define YLW_LED D3
#define GRN_LED D4
#define WHT_LED D5

const char* thingerUser = "XXXX";
const char* deviceCode = "XXXX";
const char* deviceName = "XXXX";

// WiFi access point details.
char *ssid = "XXXX";
char *password = "XXXX";

ThingerESP8266 *thing;

int allLEDs[4] = {RED_LED, YLW_LED, GRN_LED, WHT_LED};

void setup()
{
  Serial.begin(115200);

  initializeLEDs();
  connectToWiFi();

  initializeThingerAPI();

  Serial.println("Connected to Thinger");
}

// Connect to Thinger.io and initialize end points.
void initializeThingerAPI() {
  thing =  new ThingerESP8266(thingerUser, deviceName, deviceCode);

  (*thing)["led_glow"] << [](pson & in) {
    Serial.println("led_glow triggered");

    // If nothing is publised then, default the blink to WHT_LED
    if (in.is_empty()) {
      Serial.println("Empty response received from Thinger");
      blink(WHT_LED, 500, 500);
    } else {
      int priority = (int) in;
      Serial.println("Request received from Thinger ");
      Serial.println(priority);

      switch (priority) {
        case 1:
          turnLEDOnTurnOffRest(RED_LED);
          break;
        case 2:
          turnLEDOnTurnOffRest(YLW_LED);
          break;
        case 3:
          turnLEDOnTurnOffRest(GRN_LED);
          break;
        case 4:
        default:
          turnLEDOnTurnOffRest(WHT_LED);
      }
    }
  };
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wifi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    blink(ONBOARDLED, 500, 500);
    delay(500);
  }
  Serial.println("");
  Serial.println("IP Address : ");
  Serial.println(WiFi.localIP());
}

void turnLEDOnTurnOffRest(int whichLED) {
  int pinCounter = 0;
  int noOfLEDs = sizeof(allLEDs);

  // Turn off all
  while (pinCounter < noOfLEDs) {
    digitalWrite(allLEDs[pinCounter], LOW);
    pinCounter++;
  }

  // Light the correct one.
  digitalWrite(whichLED, HIGH);
}

// All the LEDs are Outputs.
void initializeLEDs() {
  pinMode(ONBOARDLED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(YLW_LED, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  pinMode(WHT_LED, OUTPUT);
}


void loop()
{
  // Keep flashing to show alive state
  blink(ONBOARDLED, 500, 500);
  (*thing).handle();
}

// Blinks an LED and again turns it off after 'x' seconds.
void blink(int pin_number, int on, int off)
{
  digitalWrite(pin_number, HIGH);
  delay(on);
  digitalWrite(pin_number, LOW);
  delay(off);
}
