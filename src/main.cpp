#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <Arduino.h>
#include <esp_ota_ops.h>
#include <Preferences.h>
#include "systemScreen.hpp"
#include "base/graphics.hpp"
#include "base/n64controller.hpp"

// void loop2(void *pvParameters);
void setup() {
    
    Serial.begin(921600);
    GFX::init();
    N64C::init();

    esp_reset_reason_t reset_reason = esp_reset_reason();
    Serial.println(reset_reason);
    if (reset_reason == ESP_RST_POWERON){
        GFX::writeText("COLD-BOOT DETECTED", 0, 0);
    }
    else{
        GFX::writeText("WARM-BOOT DETECTED", 0, 0);
    }
    GFX::updateScreen();
    delay(1000);

    // xTaskCreatePinnedToCore(
    //     loop2,     // Function to run as task
    //     "Task1",       // Name of the task
    //     10000,         // Stack size in words
    //     NULL,          // Parameter to pass
    //     24,             // Task priority
    //     NULL,        // Task handle
    //     0              // Core number (0 for Core 0)
    // );

    initSystemScreen();
    Serial.println("HI");

    // Initialize OTA
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch"; // firmware
        } else { // U_SPIFFS
            type = "filesystem"; // SPIFFS
        }
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd OTA");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
}

void loop() {
    GFX::executeRoutines();
    GFX::updateScreen();
    N64C::updateInputBuffer();
    N64C::update();
}

// void loop2(void *pvParameters){
//     while(true){
//         GFX::updateScreen();
//         vTaskDelay(1);
//     }
// }