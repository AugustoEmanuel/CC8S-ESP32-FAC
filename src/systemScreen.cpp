#include "base/graphics.hpp"
#include "base/n64controller.hpp"
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>
#include <ArduinoJson.h>
#include "lz4.h"

const char* ssid = "ESP32AE";
const char* password = "thor2014";

int selectedSoftware = 0;
int softwareCount = 0;
std::vector<String> availableSoftwares{};

const char* firmwareURL = "http://18.231.149.201:7777/download/%d";

static void downloadSoftware();
static void drawSysScreen();
static void controls(N64C_D actualState, N64C_D changed);
static void drawCursor();

Preferences preferences;
void initSystemScreen(){
    GFX::clearFrameBuffer();
    GFX::writeText("CONNECTING TO WIFI...", 0, 0);
    GFX::updateScreen();

    WiFi.begin(ssid, password);
    // Wait for the connection to establish
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    GFX::clearFrameBuffer();
    GFX::writeText("CONNECTED TO WIFI!", 0, 0);
    GFX::updateScreen();
    delay(1000);

    GFX::clearFrameBuffer();

    preferences.begin("ota_str");
    bool ota_downloaded = preferences.getBool("ota_downloaded", false);

    HTTPClient http;
    http.begin("http://18.231.149.201:7777/software");
    int httpCode = http.GET();

    GFX::writeText("CHOOSE A SOFTWARE TO DOWNLOAD", 0 , 0);
    if (httpCode == HTTP_CODE_OK) {
        int len = http.getSize();
        WiFiClient *client = http.getStreamPtr();

        if (len > 0) {
            String json = http.getString();
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, json);
            JsonArray array = doc.as<JsonArray>();

            int y = 16;
            softwareCount = array.size();
            for (JsonVariant v : array) {
                GFX::writeText(v["name"], 8, y += 8);
            }
        }
    } else {
        Serial.printf("HTTP GET failed, code: %d\n", httpCode);
    }
    http.end();
    GFX::updateScreen();

    GFX::setDrawCallback(drawSysScreen);
    N64C::setControllerCallback(controls);

    Serial.print(esp_get_free_heap_size());
}

static void drawSysScreen(){
    drawCursor();
}

static void drawCursor(){
    for(int i = 0; i < softwareCount; i++){
        int y = 24 + i*8;
        if(i == selectedSoftware) GFX::writeText(">", 0, y);
        else GFX::fillRect(0, y, 8, 8, 0x0); 
    }
}

void controls(N64C_D actualState, N64C_D changed){
    if(changed.down){
        selectedSoftware++;
        selectedSoftware %= softwareCount;
    }
    else if(changed.up){
        selectedSoftware--;
        selectedSoftware %= softwareCount;
    }
    else if(changed.A){
        downloadSoftware();
    }
}

static void downloadSoftware(){
    GFX::clearFrameBuffer();
    GFX::writeText("DOWNLOADING...", 0, 0);
    GFX::updateScreen();
    HTTPClient http;

    char donwloadFormatedUrl[50];
    sprintf(donwloadFormatedUrl, firmwareURL, selectedSoftware);

    http.begin(donwloadFormatedUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        int len = http.getSize();
        WiFiClient *client = http.getStreamPtr();

        if (len > 0) {
            Update.begin(len);
            size_t written = Update.writeStream(*client);
            if (written == len) {
                GFX::writeText("DOWNLOAD COMPLETED", 0, 8);
                GFX::updateScreen();
                preferences.putBool("ota_downloaded", true);
                Serial.println("Update complete");
            } else {
                Serial.printf("Update failed, written only: %d out of %d\n", written, len);
            }
            Update.end();
        }
    } else {
        Serial.printf("HTTP GET failed, code: %d\n", httpCode);
    }
    http.end();

    delay(1000);
    ESP.restart();
}