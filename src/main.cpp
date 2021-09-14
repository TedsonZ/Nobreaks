/**
 * ----------------------------------------------------------------------------
 * ESP32 Remote Control with WebSocket
 * ----------------------------------------------------------------------------
 * © 2020 Stéphane Calderoni
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------
#define SETOR "TESTE" // cada placa esp32 deve receber a sua identifição de setor.
#define NB_BUTTONS 2
#define ACFAIL_PIN 13
#define LOWBAT_PIN 15
#define LED_PIN   14
#define BTN_PIN   27
#define HTTP_PORT 80
IPAddress ip(192, 168, 137, 35); // cada placa esp32 deve receber o seu proprio IP.
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 0, 0);
// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// Button debouncing
const uint8_t DEBOUNCE_DELAY = 10; // in milliseconds

// WiFi credentials
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ----------------------------------------------------------------------------
// Definition of the LED component
// ----------------------------------------------------------------------------

struct Led {
    // state variables
    uint8_t pin;
    bool    on;

    // methods
    void update() {
        digitalWrite(pin, on ? HIGH : LOW);
    }
};

// ----------------------------------------------------------------------------
// Definition of the Button component
// ----------------------------------------------------------------------------

struct Button {
    // state variables
    uint8_t  pin;
    bool     lastReading;
    uint32_t lastDebounceTime;
    uint16_t state;

    // methods determining the logical state of the button
    bool pressed()                { return state == 1; }
    bool released()               { return state == 0xffff; }
    bool held(uint16_t count = 0) { return state > 1 + count && state < 0xffff; }

    // method for reading the physical state of the button
    void read() {
        // reads the voltage on the pin connected to the button
        bool reading = digitalRead(pin);

        // if the logic level has changed since the last reading,
        // we reset the timer which counts down the necessary time
        // beyond which we can consider that the bouncing effect
        // has passed.
        if (reading != lastReading) {
            lastDebounceTime = millis();
        }

        // from the moment we're out of the bouncing phase
        // the actual status of the button can be determined
        if (millis() - lastDebounceTime > DEBOUNCE_DELAY) {
            // don't forget that the read pin is pulled-up
            bool pressed = reading == LOW;
            if (pressed) {
                     if (state  < 0xfffe) state++;
                else if (state == 0xfffe) state = 2;
            } else if (state) {
                state = state == 0xffff ? 0 : 0xffff;
            }
        }

        // finally, each new reading is saved
        lastReading = reading;
    }
};

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

Led    onboard_led = { LED_BUILTIN, false };
//Led    led         = { LED_PIN, false };
Led led[] = {
    { LED_PIN, false }
};
//Button button      = { BTN_PIN, HIGH, 0, 0 };
Button button[] = {
    { BTN_PIN, HIGH, 0, 0 },
    { ACFAIL_PIN, HIGH, 0, 0 },
    { LOWBAT_PIN, HIGH, 0, 0 }
};

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");
bool acReconhecido = 0;
bool lowReconhecido = 0;
bool acSave = 0;
bool lowSave = 0;
// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Cannot mount SPIFFS volume...");
    while (1) {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
    }
  }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.config(ip, gateway, subnet); //implementado para IP FIXO//
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var) {
    //Preciso descobrir como fazer funcionar com meu codigo atual,
    //enquanto isso utilizo o JavaScript para solicitar o primeiro status.    
    //return String(var == "STATE" && led[1].on ? "on" : "off");
    return "null";
}

void onRootRequest(AsyncWebServerRequest *request) {
  //request->send(SPIFFS, "/index.html", "text/html", false, processor);
  request->send(SPIFFS, "/index.html", "text/html", false);
}

void initWebServer() {
    server.on("/", onRootRequest);
    server.serveStatic("/", SPIFFS, "/");
    server.begin();
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

void notifyClients() {    
    const String JSON_AC = "acfail";
    const String JSON_LOW = "lowbat";
    const String NORMAL  = "Normal";
    const String RECONHECIDO  = "Reconhecido";
    const String FALHA = "Falha"; 
    const String LOG = "log";
    const String LOCAL = "local";
    const String LOG_DATA = "ld";
    const String LOG_HORA = "lh";
    const String LOG_MSG = "lm";
    const uint8_t NB_LOGS = 5;

    //const uint16_t size = JSON_OBJECT_SIZE(1);
    const uint16_t size = 512;
    StaticJsonDocument<size> json;

    json[LOCAL] = SETOR;

  if(acSave != 1){
    json[JSON_AC] = NORMAL;
        acReconhecido = 0;
    }
    else if(acReconhecido){
        json[JSON_AC] = RECONHECIDO;
    }
    else{
      json[JSON_AC] = FALHA;
      }
  
  if(lowSave){
        json[JSON_LOW] = NORMAL;
        lowReconhecido = 0;
    }
    else if(lowReconhecido){
        json[JSON_LOW] = RECONHECIDO;
    }
    else{
      json[JSON_LOW] = FALHA;
      }

  for(uint8_t i = 0; i <= NB_LOGS; i++ ){
    String posicao = String(i);      
    json[LOG][LOG_DATA + posicao] = ""; //"10/09/2021";
    json[LOG][LOG_HORA + posicao] = ""; //"15:58:00";
    json[LOG][LOG_MSG + posicao] = "";  //"LOW-BATTERY Normalizado.";
  } 

  char buffer[size];
  size_t len = serializeJson(json, buffer);
  //Serial.println(len);
  //Serial.println(buffer); 
  ws.textAll(buffer, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

        const uint8_t size = JSON_OBJECT_SIZE(1);
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }

        const char *action = json["action"];
        if(strcmp(action, "obterleitura") == 0) {
            notifyClients();
        }else if(strcmp(action, "reconhecerAcfail") == 0) {
            acReconhecido = 1;
            notifyClients();
        }else if(strcmp(action, "reconhecerLowbat") == 0) {
            lowReconhecido = 1;
            notifyClients();
        }

    }
}

void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len) {

    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void leitura() {  
    
    static unsigned long previousMillis_pisca = millis();
    uint8_t intervalo_pisca = 250;
  if (millis() - previousMillis_pisca >= intervalo_pisca)  {
    previousMillis_pisca = millis();
    if(digitalRead(ACFAIL_PIN) != acSave){
        acSave = !acSave;
        notifyClients();
    }
    if (digitalRead(LOWBAT_PIN) != lowSave){
        lowSave = !lowSave;
        notifyClients();
    }
  }
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup() {
    pinMode(onboard_led.pin, OUTPUT);
    pinMode(ACFAIL_PIN, INPUT_PULLUP);
    pinMode(LOWBAT_PIN, INPUT_PULLUP);

    Serial.begin(115200); delay(500);

    initSPIFFS();
    initWiFi();
    initWebSocket();
    initWebServer();
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop(){
    ws.cleanupClients();
    leitura();
    onboard_led.on = millis() % 1000 < 50;
    //led.update();
    onboard_led.update();
}