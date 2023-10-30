#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

WiFiMulti wifiMulti;
const uint32_t connectTimeoutMs = 10000;


const char *serverUrl = "http://192.168.52.203:3000/data";
TaskHandle_t Task1;
float feelsLikeC,Td;
Adafruit_BME280 bme;

void setup() {
  Serial.begin(9600);
  //WiFi.begin(ssid, password);
  bme.begin(0x76);
  //while (WiFi.status() != WL_CONNECTED) {
   // delay(1000);
  //  Serial.println("Connecting to WiFi...");
  //}
  WiFi.mode(WIFI_STA);
  
  // Add list of wifi networks
  wifiMulti.addAP("vishesh", "1234567890");
  wifiMulti.addAP("RG1114 8261", "12345678");
  wifiMulti.addAP("R.G", "123456789");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } 
  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }
  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 
  
}

void Task1code( void * pvParameters ){
  // Serial.print("Task1 running on core ");
  // Serial.println(xPortGetCoreID());

  for(;;){
  float Tc = bme.readTemperature();
  float P = bme.readPressure() / 100.0;
  float H = bme.readHumidity();
  
  float Tf= (Tc * 9/5) + 32;
  float c1 = -42.379;
  float c2= 2.04901523;
  float c3 = 10.14333127;
  float c4 = -0.22475541;
  float c5 = -6.83783 * pow(10,(-3));
  float c6 = -5.481717 * pow(10,(-2));
  float c7 = 1.22874 * pow(10,(-3));
  float c8 = 8.5282 *pow(10,(-4));
  float c9 = -1.99 * pow(10,(-6));
  
  
  
  float e=2.718;
  float  E = 6.112 *pow( e,(17.67 * Tc / (Tc + 243.5)));
  float  es = 6.112 * pow(e,(17.67 * Tc / (Tc + 243.5)));
  float qs = (0.622 * es) / P ;
  float q = (0.622 * e) / (P - (0.378 * e));
  float RH = (q / qs) * 100;
   Td = Tc - ((100 - RH) / 5);
  float feelsLike=c1 + c2 * Tf + c3 * RH + c4 * Tf * RH + c5 * pow( Tf,2) + c6 *pow(RH,2) + c7 * pow(Tf,2) * RH + c8 * Tf * pow(RH,2) + c9 * pow(Tf,2) * pow(RH,2);
   feelsLikeC = (feelsLike - 32) * 5/9;
  Serial.print("Feels like: ");
  Serial.print(feelsLikeC);
  Serial.print("Dew point: ");
  Serial.print(Td);
  delay(5000);


    
  } 
}

void loop() {
  if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
  }
  else {
    Serial.println("WiFi not connected!");
  }
  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0;
  float humidity = bme.readHumidity();
  float altitude = bme.readAltitude(1013.25);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print(" hPa, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Altitude: ");
  Serial.print(altitude);
  Serial.println(" meters");

  
  // Send data to server
  String payload = "{\"temp\":" + String(temperature) +
                   ",\"pressure\":" + String(pressure) +
                   ",\"humidity\":" + String(humidity) +
                   ",\"altitude\":" + String(altitude) +
                   ",\"feelsLike\":"+String(feelsLikeC)+
                   ",\"dewPoint\":"+String(Td)+ "}";

  sendSensorData(payload);
  delay(10000); // Adjust delay based on your requirements
}

void sendSensorData(String payload) {
  HTTPClient http;
  
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Server response: " + response);
  } else {
    Serial.println("HTTP request failed");
  }

 http.end();
}