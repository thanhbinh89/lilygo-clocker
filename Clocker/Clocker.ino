#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "epd_driver.h"
#include "esp_adc_cal.h"

#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "logo.h"
#include "time.h"
#include "firasans.h"
#include "carbon_droid100.h"
#include "sydney_signature70.h"
#include "haha.h"
#include "hihi.h"
#include "leuleu.h"
#include "tim.h"

#define BATT_PIN 36
#define SD_MISO 12
#define SD_MOSI 13
#define SD_SCLK 14
#define SD_CS 15

uint8_t *framebuffer;
int vref = 1100;

const char *ssid = "Thanhbinh89";
const char *password = "123456789";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7*60*60;
const int daylightOffset_sec = 0;

int lastDay = -1;
int lastMin = -1;
int lastSec = -1;

void setup()
{
    Serial.begin(115200);

    // Correct the ADC reference voltage
    // esp_adc_cal_characteristics_t adc_chars;
    // esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    // if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    // {
    //     Serial.printf("\neFuse Vref:%u mV", adc_chars.vref);
    //     vref = adc_chars.vref;
    // }
    
    // Init screen control
    epd_init();

    framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!framebuffer)
    {
        Serial.println("alloc memory failed !!!");
        while (1);
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    epd_poweron();
    epd_clear();
    showLogo();
   

    // Sync NTP from Wifi
    connectWifiAndSyncNTP();
    
    hideLogo();
    epd_poweroff();
}

void loop()
{
    delay(1000);
    showLocalTime();
}

void connectWifiAndSyncNTP() {
    // Connect to Wi-Fi
    int count;
    struct tm timeinfo = {0};
    Serial.print("\nConnecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);    
    // Checking in 60s
    count = 60;
    while (WiFi.status() != WL_CONNECTED && count--)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");

    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // Checking in 3s
    count = 3;   
    while (!getLocalTime(&timeinfo) && count--)
    {
        Serial.println("Failed to obtain time");
        delay(1000);
    }

    // disconnect WiFi as it's no longer needed
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void randomXYinArea(int& rx, int& ry, int x, int y, int w, int h) {
    rx = x + millis() % w;
    ry = y + micros() % h;
}

void showLogo() {
#if 0    
    Rect_t area = {
        .x = 20,
        .y = 80,
        .width = haha_width,
        .height = haha_height,
    };
    epd_draw_grayscale_image(area, (uint8_t *)haha_data);

    area.x = 350;
    area.y = 10;
    area.width = leuleu_width;
    area.height = leuleu_height;
    epd_draw_grayscale_image(area, (uint8_t *)leuleu_data);

    area.x = 680;
    area.y = 30;
    area.width = tim_width;
    area.height = tim_height;
    epd_draw_grayscale_image(area, (uint8_t *)tim_data);

    area.x = 830;
    area.y = 390;
    area.width = hihi_width;
    area.height = hihi_height;
    epd_draw_grayscale_image(area, (uint8_t *)hihi_data);
#endif 
    Rect_t area = {
        .x = EPD_WIDTH / 2 - leuleu_width / 2,
        .y = EPD_HEIGHT / 2 - leuleu_height / 2,
        .width = leuleu_width,
        .height = leuleu_height,
    };
    epd_draw_grayscale_image(area, (uint8_t *)leuleu_data);    
}

void hideLogo() {
    Rect_t area = {
        .x = EPD_WIDTH / 2 - leuleu_width / 2,
        .y = EPD_HEIGHT / 2 - leuleu_height / 2,
        .width = leuleu_width,
        .height = leuleu_height,
    };
    epd_clear_area(area);
    // epd_clear();
}

int abs(int a, int b) {
    return (a > b) ? (a - b) : (b - a);
}

void showLocalTime()
{
    char timStr[64], dateStr[64];
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    strftime(timStr, sizeof(timStr), "%A, %B %d %Y ", &timeinfo);
    Serial.print(timStr);
    strftime(dateStr, sizeof(dateStr), "%H:%M:%S", &timeinfo);
    Serial.println(dateStr);

    // When reading the battery voltage, POWER_EN must be turned on
    epd_poweron();
    // delay(10); // Make adc measurement more accurate
    // uint16_t v = analogRead(BATT_PIN);
    // float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    // String voltage = "Voltage: " + String(battery_voltage) + "V";
    // Serial.println(voltage);

    if (lastDay != -1 && lastDay != timeinfo.tm_mday) {
        lastDay = timeinfo.tm_mday;
        connectWifiAndSyncNTP();
    }

    if (lastMin == -1 || lastMin != timeinfo.tm_min) {
        lastMin = timeinfo.tm_min;
        
        int cursor_x = 40;
        int cursor_y = 200;
        Rect_t area = {
            .x = 0,
            .y = 0,
            .width = EPD_WIDTH,
            .height = EPD_HEIGHT / 2,
        };
        epd_clear_area(area);        
        writeln((GFXfont *)&sydney_signature70, (char *)timStr, &cursor_x, &cursor_y, NULL);
    }   
    
    if ( lastSec == -1 || timeinfo.tm_sec % 10 == 0) {
        lastSec = timeinfo.tm_sec;

        int cursor_x = 50;
        int cursor_y = 450;   
        Rect_t area = {
            .x = 0,
            .y = EPD_HEIGHT / 2,
            .width = EPD_WIDTH,
            .height = EPD_HEIGHT / 2,
        };
        epd_clear_area(area);  
        writeln((GFXfont *)&carbon_droid100, (char *)dateStr, &cursor_x, &cursor_y, NULL);
    }

    // // There are two ways to close

    // // It will turn off the power of the ink screen, but cannot turn off the blue LED light.
    // // epd_poweroff();

    // //It will turn off the power of the entire
    // // POWER_EN control and also turn off the blue LED light
    epd_poweroff_all();
}