#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// SSID and Password

const char* ssid = "PLDTHOMEFIBRec068";
const char* password = "PLDTWIFI39h3w";

const char*base_rest_url = "http://192.168.1.9:5000/"; // Replace with your MongoDB Atlas endpoint

WiFiClient client;
HTTPClient http;

// Read interval
unsigned long previousMillis = 0;
const long readInterval = 5000;
// LED Pin
const int LED_PIN = 2;

// Struct to represent our LED  record
struct LED
{
  char sensor_id[10];
  char description[20];
  char location[20];
  bool enabled;
  char type[20];
  char status[10];
};

// Size of the JSON document. Use the ArduinoJSON JSONAssistant
const int JSON_DOC_SIZE = 768;

// HTTP GET Call
StaticJsonDocument<JSON_DOC_SIZE> callHTTPGet(const char *sensor_id)
{
  char rest_api_url[200];
  // Calling our API server
  sprintf(rest_api_url, "%sapi/sensors?sensor_id=%s", base_rest_url, sensor_id);
  Serial.println(rest_api_url);

  http.useHTTP10(true);
  http.begin(client, rest_api_url);
  http.addHeader("Content-Type", "application/json");
  http.GET();

  StaticJsonDocument<JSON_DOC_SIZE> doc;
  // Parse response
  DeserializationError error = deserializeJson(doc, http.getStream());

  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return doc;
  }

  http.end();
  return doc;
}

// Extract LED records
LED extractLEDConfiguration(const char *sensor_id)
{
  StaticJsonDocument<JSON_DOC_SIZE> doc = callHTTPGet(sensor_id);
  if (doc.isNull() || doc.size() > 1)
    return {};
  for (JsonObject item : doc.as<JsonArray>())
  {

    const char *sensorId = item["sensor_id"];      // "led_1"
    const char *description = item["description"]; // "This is our LED"
    const char *location = item["location"];       // "Inside the bedroom"
    bool enabled = item["enabled"];                // true
    const char *type = item["type"];               // "toggle"
    const char *status = item["status"];           // "HIGH"

    LED ledTemp = {};
    strcpy(ledTemp.sensor_id, sensorId);
    strcpy(ledTemp.description, description);
    strcpy(ledTemp.location, location);
    ledTemp.enabled = enabled;
    strcpy(ledTemp.type, type);
    strcpy(ledTemp.status, status);

    return ledTemp;
  }
  return {}; // or LED{}
}

// Convert HIGH and LOW to Arduino compatible values
int convertStatus(const char *value)
{
  if (strcmp(value, "HIGH") == 0)
  {
    Serial.println("Setting LED to HIGH");
    return HIGH;
  }
  else
  {
    Serial.println("Setting LED to LOW");
    return LOW;
  }
}

// Set our LED status
void setLEDStatus(int status)
{
  Serial.printf("Setting LED status to : %d", status);
  Serial.println("");
  digitalWrite(LED_PIN, status);
}

void setup()
{
  
  Serial.begin(9600);
  Serial.print("Hello");
  for (uint8_t t = 2; t > 0; t--)
  {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //  Start DHT Sensor readings
  // Setup LED
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= readInterval)
  {
    // save the last time
    previousMillis = currentMillis;

    Serial.println("---------------");
    // Read our configuration for our LED
    LED ledSetup = extractLEDConfiguration("led_1");
    Serial.println(ledSetup.sensor_id);
    Serial.println(ledSetup.description);
    Serial.println(ledSetup.location);
    Serial.println(ledSetup.enabled);
    Serial.println(ledSetup.type);
    Serial.println(ledSetup.status);
    setLEDStatus(convertStatus(ledSetup.status)); // Set LED value

    
  }

}