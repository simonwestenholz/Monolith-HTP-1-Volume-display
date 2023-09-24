#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketClient.h>
#include "rm67162.h"
#include <TFT_eSPI.h>


const char* ssid     = "xxx";
const char* password = "xxx";
char path[] = "/ws/controller";
char host[] = "192.168.1.xxx";

WebSocketClient webSocketClient;
WiFiClient client;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

#define up 21
#define down 0
#define led 38
bool deb = 0;
bool deb2 = 0;
byte bright = 5;
byte brightness[7] = { 7, 20, 25, 30, 50, 55, 60 };
bool ledON = false;

String vol = "";
int volume;
unsigned long lastTime = 0;
unsigned long timerDelay2 = 3000;

void setup() {
  Serial.begin(115200);
  pinMode(up, INPUT_PULLUP);
  pinMode(down, INPUT_PULLUP);
  pinMode(led, OUTPUT);
  sprite.createSprite(536, 240);
  sprite.setSwapBytes(1);

  WiFi.setHostname("HTP-1 Volume display");

  rm67162_init();
  lcd_setRotation(1);
  lcd_brightness(brightness[3]);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (client.connect(host, 80)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
    return;
  }

  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {
    Serial.println("Handshake failed.");
    return;
  }
}

void draw() {
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(TFT_WHITE,TFT_BLACK);
  sprite.setTextSize(5);
  sprite.drawString(vol, 0, 0, 7);
  lcd_PushColors(0, 0, 536, 240, (uint16_t*)sprite.getPointer());
}

void readButtons(){
  if (digitalRead(up) == 0) {
    if (deb == 0) {
      deb = 1;
      bright++;
      if (bright == 7) bright = 0;
      lcd_brightness(brightness[bright]);
    }
  } else deb = 0;

  if (digitalRead(down) == 0) {
    if (deb2 == 0) {
      deb2 = 1;
      ledON = !ledON;
      digitalWrite(led, ledON);
    }
  } else deb2 = 0;
}


void loop() {
  readButtons();
  String data;

  if (client.connected()) {
    
    webSocketClient.getData(data);
    if (data.length() > 0) {
      Serial.print("Received data: ");
      Serial.println(data);
      
      // Check for the existence of the non-JSON prefix and remove it
      String prefix = "msoupdate ";
      if (data.startsWith(prefix)) {
        data = data.substring(prefix.length());
      }

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, data);

      if (!error) {
        // Check if the parsed JSON is an array
        if (doc.is<JsonArray>()) {
          JsonArray array = doc.as<JsonArray>();
          
          // Iterate over the elements of the array
          for (JsonObject elem : array) {
            // Check if the current object has a property "path" with the value "/volume"
            if (elem["path"] == "/volume") {
              // Extract and save the volume value
              volume = elem["value"];
              Serial.print("Volume: ");
              Serial.println(volume);
              vol = String(volume + 7); // adding offset for reference level
              draw();
              lastTime = millis();
            }
          }
        }
      } else {
        // Print an error message if deserialization fails
        Serial.print("Deserialization error: ");
        Serial.println(error.c_str());
      }
    }
    
  } else {
    Serial.println("Client disconnected.");
    while (1) {
      // Hang on disconnect.
    }
  }
  
  if ((millis() - lastTime) < timerDelay2){
    lcd_brightness(brightness[bright]);
  } else if ((millis() - lastTime) > timerDelay2) {
    lcd_brightness(brightness[0]);
  }

  // Wait to fully let the client disconnect
  // delay(3000);
  
}
