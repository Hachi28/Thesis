#include <HTTPClient.h>
#include <ArduinoJson.h>

// SSID and Password

const char* ssid = "PLDTHOMEFIBRec068";
const char* password = "PLDTWIFI39h3w";

const char*base_rest_url = "http://192.168.1.8:5000/"; // Replace with your MongoDB Atlas endpoint

WiFiClient client;
HTTPClient http;

// Read interval
unsigned long previousMillis = 0;
const long readInterval = 5000;
// LED Pin
const int LED_PIN = 26;
const int numLoops = 3; // Number of loops
float lightReadings[numLoops]; // Array to store light readings

const float pathLength = 6.5; // Path length (in centimeters)
const float intensityWithoutSmoke = 500; // Maximum intensity without smoke
const float smokeFactor = 0.155; // Empirical factor based on smoke type and sensor characteristics

char dhtObjectId[30];

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

struct DHT22Readings
{
  float opacity;
  float average;
};

// Size of the JSON document. Use the ArduinoJSON JSONAssistant
const int JSON_DOC_SIZE = 384;

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

void sendDHT22Readings(const char *objectId, DHT22Readings dhtReadings)
{
  char rest_api_url[200];
  // Calling our API server
  sprintf(rest_api_url, "%sapi/sensors/%s", base_rest_url, objectId);
  Serial.println(rest_api_url);

  // Prepare our JSON data
  String jsondata = "";
  StaticJsonDocument<JSON_DOC_SIZE> doc;
  JsonObject readings = doc.createNestedObject("readings");
  readings["opacity"] = dhtReadings.opacity;
  
  serializeJson(doc, jsondata);
  Serial.println("JSON Data...");
  Serial.println(jsondata);

  http.begin(client, rest_api_url);
  http.addHeader("Content-Type", "application/json");

  // Send the PUT request
  int httpResponseCode = http.PUT(jsondata);
  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
    http.end();
  }
}

// Get DHT22 ObjectId
void getDHT22ObjectId(const char *sensor_id)
{
  StaticJsonDocument<JSON_DOC_SIZE> doc = callHTTPGet(sensor_id);
  if (doc.isNull() || doc.size() > 1)
    return;
  for (JsonObject item : doc.as<JsonArray>())
  {
    Serial.println(item);
    const char *objectId = item["_id"]["$oid"]; // "dht22_1"
    strcpy(dhtObjectId, objectId);

    return;
  }
  return;
}

DHT22Readings readDHT22()
{
 for (int i = 0; i < numLoops ; ++i) {

    // Read analog value from the photoresistor
    float lightIntensity = analogRead(32);

    // Calculate smoke opacity
    // Calculate percentage
      float iniopac = (lightIntensity/intensityWithoutSmoke);
      float opacity =  1 - iniopac;

    //Save reading
    lightReadings[i] = opacity;

    // Print the current reading
      Serial.print("Loop ");
      Serial.print(i + 1);
      Serial.print(": Light percentage = ");
      Serial.println(opacity);
    
      delay(5000); // Delay for 1 second
      //Print Opacity value to LCD

    
    }
      // Calculate average
    float total = 0;
    for (int i = 0; i < numLoops; ++i) {
      total += lightReadings[i];
    }
    float average = total / numLoops;
  
    // Wait before starting the next loop
    delay(5000); // Delay for 10 seconds
    Serial.print("Opacity: ");
    Serial.println(average);

  return {average};
}

// Extract LED records
LED extractLEDConfiguration(const char *sensor_id)
{
  StaticJsonDocument<JSON_DOC_SIZE> doc = callHTTPGet(sensor_id);
  if (doc.isNull() || doc.size() > 1)
    return {};
  for (JsonObject item : doc.as<JsonArray>())
  {

    const char *sensorId = item["sensor_id"];      // "led_1"            // "toggle"
    const char *status = item["status"];           // "HIGH"

    LED ledTemp = {};
    strcpy(ledTemp.sensor_id, sensorId);
    strcpy(ledTemp.status, status);

    return ledTemp;
  }
  return {}; // or LED{}
}

// Convert HIGH and LOW to Arduino compatible values
int convertStatus(const char *value)
{
  if (strcmp(value, "ON") == 0)
  {
    Serial.println("Setting LED to ON");
    return HIGH;
  }
  else
  {
    Serial.println("Setting LED to OFF");
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
    LED ledSetup = extractLEDConfiguration("button");
    Serial.println(ledSetup.sensor_id);
    Serial.println(ledSetup.status);
    setLEDStatus(convertStatus(ledSetup.status)); // Set LED value

    Serial.println("---------------");
    if (digitalRead(LED_PIN) == HIGH) {
      DHT22Readings readings = readDHT22();
      // Send our Photoresistor sensor readings
      // Locate the ObjectId of our Photoresistor document in MongoDB
      Serial.println("---------------");
      Serial.println("Sending latest Opacity readings");

      sendDHT22Readings("662cbbc7530a2d3a53323ed9", readings);
      setLEDStatus(LOW);

      delay(10000);
    } else {
      Serial.println("The sequence if Offline");
      
    }
   
  }   
}