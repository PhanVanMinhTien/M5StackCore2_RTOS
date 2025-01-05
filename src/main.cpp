//LIBRARY
#include <M5Unified.h>
#include <Arduino.h>
#include <DHT20.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "Adafruit_NeoPixel.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

//PIN DEFINE
#define light_sensor_pin 34
#define rgb_pin 13
#define relay_pin 27
#define soil_sensor_pin 36
//WIFI
#define WLAN_SSID "Happy New Year 2021"
#define WLAN_PASS "phanvanminhtien"
//ADAFRUIT
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "wanu2310"
#define AIO_KEY         "aio_CBGY18cl8vZzSvxV4pMA330FqWpw"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);
/****************************** Feeds ***************************************/

// Setup a feed called 'time' for subscribing to current time
Adafruit_MQTT_Subscribe timefeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");

// Setup a feed called 'slider' for subscribing to changes on the slider
Adafruit_MQTT_Subscribe slider = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/slider", MQTT_QOS_1);

// Setup a feed called 'onoff' for subscribing to changes to the button
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff", MQTT_QOS_1);

// Setup a feed called 'photocell' for publishing.
Adafruit_MQTT_Publish sensory = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sensory");
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sensors.temperature");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sensors.humidity");
Adafruit_MQTT_Publish soil = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sensors.soil");
Adafruit_MQTT_Publish light = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/sensors.light");


// Define your tasks here
void TaskTemperatureHumidity(void *pvParameters);
void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);
void TaskButtonAndLED(void *pvParameters);
void Task_Publish(void *pvParameters);
void Task_LCD(void *pvParameters);

//
AsyncWebServer server(80);

//Define your components here
DHT20 dht20;
Adafruit_NeoPixel pixels1(4, rgb_pin, NEO_GRB + NEO_KHZ800);



void slidercallback(double x) {
  Serial.print("Hey we're in a slider callback, the slider value is: ");
  Serial.println(x);
}

void onoffcallback(char *data, uint16_t len) {
  Serial.print("Hey we're in a onoff callback, the button value is: ");
  Serial.println(data);
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(10000);  // wait 10 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      Serial.println("Can't connect to MQTT, running locally");
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

//----------------------------------SETUP----------------------------------------//
void setup() {
  // Initialize M5StackCore2
  Serial.begin(115200);
  M5.begin();
  dht20.begin();
  pixels1.begin();
  // 
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("M5Stack Core2 Ready!");
  delay(2000);
  M5.Lcd.clear();

int wifi_retry = 0;
WiFi.begin(WLAN_SSID, WLAN_PASS);
  delay(2000);

  while (WiFi.status() != WL_CONNECTED) {
    wifi_retry++;
    delay(500);
    Serial.print("Retry: "); Serial.println(wifi_retry/2);
    if (wifi_retry/2 >= 5) {
      wifi_retry = 0;
      break;
    }
  }

  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  slider.setCallback(slidercallback);
  onoffbutton.setCallback(onoffcallback);
  mqtt.subscribe(&slider);
  mqtt.subscribe(&onoffbutton);
  
  // ADD TASKS HERE
  xTaskCreate( TaskTemperatureHumidity, "Task Temperature" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskLightAndLED, "Task Light LED" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskSoilMoistureAndRelay, "Task Soild Relay" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( Task_LCD, "Task LCD" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate(Task_Publish, "Task Publish", 2048, NULL, 2, NULL);


  // Mount SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Lỗi khởi tạo SPIFFS");
    return;
  }
  // Định tuyến file tĩnh
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
    float temperature = dht20.getTemperature();
    float humidity = dht20.getHumidity();
    int light = analogRead(light_sensor_pin);  
    int moisture = analogRead(soil_sensor_pin);

    // Kiểm tra nếu có lỗi trong việc đọc cảm biến DHT20
    if (isnan(temperature) || isnan(humidity)) {
        request->send(500, "application/json", "{\"error\":\"Failed to read sensor data\"}");
        return;
    }

    // Tạo JSON chứa dữ liệu từ các cảm biến
    String json = "{\"temperature\":" + String(temperature) + 
                  ",\"humidity\":" + String(humidity) +
                  ",\"light\":" + String(light) +
                  ",\"moisture\":" + String(moisture) + "}";
    request->send(200, "application/json", json);
});

    // Start the server
    server.begin();
    Serial.printf("Basic Multi-Threading Arduino Example\n"); 
}

void loop() {
  MQTT_connect();
  mqtt.processPackets(10000);
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}


/*----------------------------------------------------------------------*/
/*-------------------------------- Tasks -------------------------------*/
/*----------------------------------------------------------------------*/
void TaskTemperatureHumidity(void *pvParameters) {
  //uint32_t blink_delay = *((uint32_t *)pvParameters);
  while(1) {    
    // READ SENSOR
    dht20.read();    
    float temperature = dht20.getTemperature();
    float humidity = dht20.getHumidity();        
    //Serial.println("Task Temperature and Humidity");
    //Serial.println(temperature);
    //Serial.println(humidity);

    //delay(1000);
    vTaskDelay(1000);
  }
}

void TaskLightAndLED(void *pvParameters) {
  uint32_t blink_delay = *((uint32_t *)pvParameters);
  while(1) { 
    //Serial.println("Task Light and LED");
    //Serial.println(analogRead(light_sensor_pin));
    
    if(analogRead(light_sensor_pin) < 500){
      pixels1.setPixelColor(0, pixels1.Color(6, 10, 138));
      pixels1.setPixelColor(1, pixels1.Color(6, 10, 138));
      pixels1.setPixelColor(2, pixels1.Color(6, 10, 138));
      pixels1.setPixelColor(3, pixels1.Color(6, 10, 138));
      pixels1.show();
    } 
    if(analogRead(light_sensor_pin) > 650){
      pixels1.setPixelColor(0, pixels1.Color(0,0,0));
      pixels1.setPixelColor(1, pixels1.Color(0,0,0));
      pixels1.setPixelColor(2, pixels1.Color(0,0,0));
      pixels1.setPixelColor(3, pixels1.Color(0,0,0));
      pixels1.show();
    }
    //delay(1000);

    vTaskDelay(1000);
  }
}

int count_test_rgb_button = 0;
void TaskButtonAndLED(void *pvParameters) {
  while(1){
    count_test_rgb_button = (count_test_rgb_button + 1)%20;
    if (count_test_rgb_button == 0){
      //Serial.print("D1= "); Serial.println(analogRead(34));
      //Serial.print("D2= "); Serial.println(analogRead(35));
      //Serial.println(" ");
    }
    if (analogRead(34) <= 10 && analogRead(35) <= 0) {
      Serial.println("Button 1 & 2 pressed");
      pixels1.setPixelColor(0, pixels1.Color(6, 10, 138));
      pixels1.setPixelColor(1, pixels1.Color(0,0,0));
      pixels1.setPixelColor(2, pixels1.Color(6, 10, 138));
      pixels1.setPixelColor(3, pixels1.Color(0,0,0));
      pixels1.show();
    }
    else if (analogRead(34) <= 10 && analogRead(35) > 10) {
      Serial.println("Button 1 pressed");
      pixels1.setPixelColor(0, pixels1.Color(6, 10, 138));
      pixels1.setPixelColor(1, pixels1.Color(0,0,0));
      pixels1.setPixelColor(2, pixels1.Color(0,0,0));
      pixels1.setPixelColor(3, pixels1.Color(0,0,0));
      pixels1.show();
    }
    else if (analogRead(34) > 10 && analogRead(35) <= 10) {
      Serial.println("Button 2 pressed");
      pixels1.setPixelColor(0, pixels1.Color(0,0,0));
      pixels1.setPixelColor(1, pixels1.Color(0,0,0));
      pixels1.setPixelColor(2, pixels1.Color(6, 10, 138));
      pixels1.setPixelColor(3, pixels1.Color(0,0,0));
      pixels1.show();
    }
    else {
      pixels1.setPixelColor(0, pixels1.Color(0,0,0));
      pixels1.setPixelColor(1, pixels1.Color(0,0,0));
      pixels1.setPixelColor(2, pixels1.Color(0,0,0));
      pixels1.setPixelColor(3, pixels1.Color(0,0,0));
      pixels1.show();
    }
    vTaskDelay(50);
    //delay(50);
  }
}

void TaskSoilMoistureAndRelay(void *pvParameters) {
  //uint32_t blink_delay = *((uint32_t *)pvParameters);
  pinMode(relay_pin, OUTPUT);

  while(1) {                 
    Serial.println("Task Soild and Relay");
    Serial.println(analogRead(soil_sensor_pin));
    
    if(analogRead(soil_sensor_pin) > 80){
      digitalWrite(relay_pin, LOW);
      //Serial.println("Tat may bom");
    }
    if(analogRead(soil_sensor_pin) < 60){
      digitalWrite(relay_pin, HIGH);
      //Serial.println("Bat may bom");
    }
    //delay(1000);
    // Display data on the M5 LCD screen
    vTaskDelay(1000);
  }
}

// FUNCTION FOR PUBLISHING DATA TO ADAFRUIT
float temperature_pub = 0;
float humidity_pub = 0;
uint32_t light_pub = 0;
uint32_t soil_pub = 0;
int count_publish = 0;

void Task_Publish(void *pvParameters) {
  while(1){
    count_publish++;
    if (count_publish >= 10){ //send data every 10s
      count_publish = 0;
      temperature_pub = dht20.getTemperature();
      temperature.publish(temperature_pub);
      humidity_pub = dht20.getHumidity();
      humidity.publish(humidity_pub);
      light_pub = analogRead(light_sensor_pin);
      light.publish(light_pub);
      soil_pub = analogRead(soil_sensor_pin);
      soil.publish(soil_pub);
      Serial.println("Published to mqtt successfully!");
    }
    //delay(1000);
    vTaskDelay(1000);
  }
}

void Task_LCD(void *pvParameters){
  while(1){
    M5.Lcd.fillScreen(TFT_BLACK);

    // Display temperature
    M5.Lcd.setCursor(10, 20);
    M5.Lcd.printf("Temperature: %.2f C\n", dht20.getTemperature());

    // Display humidity
    M5.Lcd.setCursor(10, 60);
    M5.Lcd.printf("Humidity: %.2f %%\n", dht20.getHumidity());

    // Display light sensor value
    M5.Lcd.setCursor(10, 100);
    M5.Lcd.printf("Light: %d\n", analogRead(light_sensor_pin));

    // Display soil sensor value
    M5.Lcd.setCursor(10, 140);
    M5.Lcd.printf("Soil: %d\n", analogRead(soil_sensor_pin));
    vTaskDelay(1000);
  }
}
