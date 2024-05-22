/*******************************************************************
    Hello World for the ESP32 Cheap Yellow Display.

    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// Make sure to copy the UserSetup.h file into the library as
// per the Github Instructions. The pins are defined in there.

// Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
// Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
// Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
// Note the following larger fonts are primarily numeric only!
// Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
// Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
// Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define ARDUINOJSON_USE_LONG_LONG 1

#include <ESPmDNS.h>
// #include <NetworkUdp.h>
#include <ArduinoOTA.h>

#include <TFT_eSPI.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <vector>
#include <map>

#include "arduino_secrets.h"

#include <XPT2046_Touchscreen.h>

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// ----------------------------

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);


// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
//https://github.com/Bodmer/TFT_eSPI

TFT_eSPI tft = TFT_eSPI();

// WiFiManager wifiManager;

// long devices_update_interval_ms = 6000;
long devices_update_interval_ms = 60000;
long entries_update_interval_ms = 10000;


int sgv;
int sgv_delta;
long long sgv_ts_ms;

struct tm tm = {0};



#define LCD_BACK_LIGHT_PIN 21

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0     0

// use 12 bit precission for LEDC timer
#define LEDC_TIMER_12_BIT  12

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

// Arduino like analogWrite
// value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 4095 from 2 ^ 12 - 1
  uint32_t duty = (4095 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

bool update_nightscout_entries() {
  
  HTTPClient http;
  http.begin(NIGHTSCOUT_ENTRIES_URL);

  /* get date: header ; it's lowercase in nightscout */
  const char* headerKeys[] = {"date"};
  const size_t headerKeysCount = 1;
  http.collectHeaders(headerKeys, headerKeysCount);
  http.addHeader("Accept", "application/json");
  int retcode = http.GET();
  if (retcode != 200) {
    Serial.print("HTTP error: ");
    Serial.println(retcode);
    return false;
  }
  JsonDocument doc;
  String response = http.getString();
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return false;
  }
  if (doc.size() == 0) {
    Serial.println("No entries found");
    return false;
  }
  JsonObject entry = doc[0];
  int reading = entry["sgv"];
  if (entry["delta"].isNull()) {
    Serial.println("WARNING delta is empty!");
    return false;
  }
  float delta = entry["delta"];
  long long reading_ms = entry["mills"];
  // long reading_sec = reading_ms / 1000;
  sgv=reading;
  sgv_delta=round(delta);
  sgv_ts_ms=reading_ms;

  if (http.header("date") == NULL) {
    Serial.println("WARNING date is empty!");
  } else {
    const char* date = http.header("date").c_str();
    strptime(date, "%a,%d %b %Y %H:%M:%S GMT", &tm);
    Serial.print("Date received was: ");
    Serial.println(date);
    set_system_time_from_tm();
    Serial.print("System time is now: ");
    time_t t = mktime(&tm);
    Serial.println(ctime(&t));
  }
  http.end();
  return true;
}

typedef struct device {
  char device[50];
  int battery;
  // char type[50];
  long long mills;
  bool set;
} device;

#define MAX_DEVICES 10
device devices[MAX_DEVICES];
device local_devices[MAX_DEVICES];

bool update_nightscout_devices() {
  /* Sample response:
  
  [
  {
    "_id": "66228c84f2d13c5e4d16fe9d",
    "device": "Google Pixel 6",
    "uploader": {
      "battery": 24,
      "type": "PHONE"
    },
    "created_at": "2024-04-19T15:23:48.942Z",
    "utcOffset": 0,
    "mills": 1713540228942
  },
  {
    "_id": "66228b91f2d13c5e4d16fe9c",
    "device": "Google Pixel 3",
    "uploader": {
      "battery": 97,
      "type": "PHONE"
    },
    "created_at": "2024-04-19T15:19:45.414Z",
    "utcOffset": 0,
    "mills": 1713539985414
  }
]*/

  HTTPClient http;
  http.begin(NIGHTSCOUT_DEVICES_URL);
    /* get date: header; it's lowercase in nightscout */
  const char* headerKeys[] = {"date"};
  const size_t headerKeysCount = 1;
  http.collectHeaders(headerKeys, headerKeysCount);
  http.addHeader("Accept", "application/json");
  int retcode = http.GET();
  if (retcode != 200) {
    Serial.print("HTTP error: ");
    Serial.println(retcode);
    return false;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, http.getStream());
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return false;
  }
  for (int i = 0; i < MAX_DEVICES; i++) {
    local_devices[i].set = false;
  }
  JsonArray devices_json = doc.as<JsonArray>();
  for (JsonObject device_json : devices_json) {
    const char* device_name = device_json["device"];
    int device_id = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
      if (local_devices[i].set && strcmp(local_devices[i].device, device_name) == 0) {
        device_id = i;
        Serial.print("Updating device ");
        Serial.print(device_name);
        Serial.print(" at index ");
        Serial.println(i);
        break;
      }
      if (local_devices[i].set == false) {
        strcpy(local_devices[i].device, device_name);
        local_devices[i].set = true;
        device_id = i;
        Serial.print("Adding device ");
        Serial.print(device_name);
        Serial.print(" at index ");
        Serial.println(i);
        break;
      }
    }
    if (device_id == -1) {
      Serial.println("WARNING: too many devices");
      continue;
    }
    int battery = device_json["uploader"]["battery"];
    // strcpy(d.type, device_json["uploader"]["type"]);
    Serial.print("Device ID: ");
    Serial.print(device_id);
    Serial.print(" Device: ");
    Serial.print(local_devices[device_id].device);
    Serial.print(" Battery: ");
    Serial.println(battery);
    // Serial.print(" Type: ");
    // Serial.println(d.type);
    // Serial.print(" Mills s: ");
    // Serial.print(long_ms);
    // Serial.print(" Mills l: ");
    // Serial.println(d.mills);

    if (device_json.containsKey("mills") == false) {
      Serial.println("WARNING mills is not present!");
      // local_devices[device_id].mills = 0;
    } else {
      long long new_mills = device_json["mills"];
      if (local_devices[device_id].mills <= new_mills) {
        Serial.print("Updating mills from ");
        Serial.print(local_devices[device_id].mills);
        Serial.print(" to ");
        Serial.println(new_mills);
        local_devices[device_id].mills = new_mills;
        local_devices[device_id].battery = battery;
      }
    }    
    if (local_devices[device_id].mills == 0) {
      Serial.println("WARNING mills is 0!");
    }
  }

  for (int i = 0; i < MAX_DEVICES; i++) {
    devices[i] = local_devices[i];
  }
    


  if (http.header("date") == NULL) {
    Serial.println("WARNING date is empty!");
  } else {
    const char* date = http.header("date").c_str();
    strptime(date, "%a, %d %b %Y %H:%M:%S GMT", &tm);
    Serial.print("Date received was: ");
    Serial.println(date);
    set_system_time_from_tm();
    Serial.print("System time is now: ");
    time_t t = mktime(&tm);
    Serial.println(ctime(&t));
  }
  http.end();

  return true;
}

void set_system_time_from_tm() {
  struct timeval tv = {0};
  tv.tv_sec = mktime(&tm);
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
}

long long cur_ms() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000ll + tv.tv_usec / 1000ll;
}

char ota_status[100];
int ota_progress = 0;
long ota_last_progress_ms = 0;
bool prev_wifi_connected = false;

void draw_header() {
  bool wifi_connected = WiFi.status() == WL_CONNECTED;
  if (wifi_connected == prev_wifi_connected && ota_last_progress_ms == 0) {
    return;
  }
  prev_wifi_connected = wifi_connected;
  tft.setTextSize(1);
  tft.fillRect(0, 0, 320, 30, TFT_BLACK);
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_DARKGREEN, TFT_BLACK);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("Connected", 320, 0, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(WiFi.localIP().toString(), 0, 0, 2);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("Not connected", 320, 0, 2);
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  if (ota_last_progress_ms &&  millis() - ota_last_progress_ms < 5000) {
    tft.drawString(ota_status, 0, 0, 2);
  } else {
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);    
    tft.drawString("NightScout", 160, 0, 2);
    ota_last_progress_ms = 0;
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
}

void setup() {
  Serial.begin(115200);
  // Start the tft display and set it to black
  tft.init();
  //tft.invertDisplay(1); //If you have a CYD2USB - https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/cyd.md#my-cyd-has-two-usb-ports
  tft.setRotation(1); //This is the display in landscape
  
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);




  // Setting up the LEDC and configuring the Back light pin
  // NOTE: this needs to be done after tft.init()
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(LCD_BACK_LIGHT_PIN, LEDC_CHANNEL_0);
  ledcAnalogWrite(LEDC_CHANNEL_0, 100);

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int x = 5;
  int y = 0;
  int fontNum = 2; 
  tft.drawString("Hello", x, y, fontNum); // Left Aligned
  x = 320 /2;
  y = 0;
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawCentreString("NightScout", x, y, fontNum);
  // wifiManager.setTimeout(180); // 3 minutes
  // wifiManager.autoConnect();
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");


    ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      snprintf(ota_status, sizeof(ota_status), "Start updating %s", type.c_str());
      ota_last_progress_ms = millis();
      draw_header();
    })
    .onEnd([]() {
      Serial.println("\nEnd");
      strncpy(ota_status, "End", sizeof(ota_status));
      ota_last_progress_ms = millis();
      draw_header();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      snprintf(ota_status, sizeof(ota_status), "Progress: %u%%", (progress / (total / 100)));
      ota_last_progress_ms = millis();
      draw_header();
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
        strncpy(ota_status, "Auth Failed", sizeof(ota_status));
        ota_last_progress_ms = millis();
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
        strncpy(ota_status, "Begin Failed", sizeof(ota_status));
        ota_last_progress_ms = millis();
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
        strncpy(ota_status, "Connect Failed", sizeof(ota_status));
        ota_last_progress_ms = millis();
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
        strncpy(ota_status, "Receive Failed", sizeof(ota_status));
        ota_last_progress_ms = millis();
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
        strncpy(ota_status, "End Failed", sizeof(ota_status));
        ota_last_progress_ms = millis();
      }
      draw_header();
    });

  ArduinoOTA.begin();

}

long prev_draw_time_ms = 0;

void maybe_draw_time() {
  if (prev_draw_time_ms == 0 || (millis() - prev_draw_time_ms > 1000) && (cur_ms() / 1000) % 60 == 0) {
    always_draw_time();
    prev_draw_time_ms = millis();
  }

}

void always_draw_time() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(BR_DATUM);
  tft.setTextSize(2);
  char buf[100];

  setenv("TZ", "America/New_York", 1);
  tzset(); // Initialize timezone data
  time_t aTime = time(NULL); // get the time - this is GMT based.
  // hardcode 240min offset
  aTime -= 240 * 60;
  struct tm retTime;
  localtime_r(&aTime, &retTime); // Convert time into current timezone.


  strftime(buf, sizeof(buf), "%H:%M", &retTime);

  setenv("TZ", "UTC", 1);
  tzset();
  tft.drawString(buf, 320, 150, 4);
}

long long prev_sgv_ts_ms = 0;
long prev_sgv_age_min = 0;
// battery will be ignored since we don't have ts for that
// and only drawn on sgv change

void draw_mem() {
  char buf[100];
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(BR_DATUM);
  tft.setTextSize(1);
  snprintf(buf, sizeof(buf), "RAM = %d", esp_get_free_heap_size());
  tft.drawString(buf, 320, 180, 2);
  snprintf(buf, sizeof(buf), "RAM2 = %d", uxTaskGetStackHighWaterMark(NULL));
  tft.drawString(buf, 320, 200, 2);
  snprintf(buf, sizeof(buf), "RAM3 = %d", ESP.getFreeHeap());
  tft.drawString(buf, 320, 220, 2);
  snprintf(buf, sizeof(buf), "RAM4 = %d", xPortGetFreeHeapSize());
  tft.drawString(buf, 320, 240, 2);
}

void draw_nightscout_data() {
  long sgv_age_min = (cur_ms() - sgv_ts_ms) / 60000ll;
  if (sgv_ts_ms == prev_sgv_ts_ms && sgv_age_min == prev_sgv_age_min) {
    return;
  }

  prev_sgv_ts_ms = sgv_ts_ms;  
  prev_sgv_age_min = sgv_age_min;
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char buf[200];
  /*
  if (sgv_delta >= 0) {
    snprintf(buf, sizeof(buf), "%d +%d", sgv, sgv_delta);
  } else {
    snprintf(buf, sizeof(buf), "%d %d", sgv, sgv_delta);
  }
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(2);
  tft.drawString(buf, 160, 20, 6);
  */

  tft.fillRect(0, 30, 320, 210, TFT_BLACK);

  snprintf(buf, sizeof(buf), "%d", sgv);
  tft.setTextSize(2);
  if (sgv >= 70 && sgv <= 180) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  }
  tft.drawString(buf, 20, 30, 6);

  if (sgv_delta >= 0) {
    snprintf(buf, sizeof(buf), "+%d", sgv_delta);
  } else {
    snprintf(buf, sizeof(buf), "%d", sgv_delta);
  }
  tft.setTextSize(3);
  if (abs(sgv_delta) < 10) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else if (abs(sgv_delta) < 20) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  }
  tft.setTextDatum(TR_DATUM);
  tft.drawString(buf, 300, 40, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setTextSize(1);

  tft.setTextDatum(TL_DATUM);
  if (sgv_ts_ms) {
    snprintf(buf, sizeof(buf), "%dm", sgv_age_min);
  }
  tft.setTextSize(2);
  tft.drawString(buf, 5, 120, 4);
  tft.setTextSize(1);
  int y = 180;
  for (int i = 0; i < MAX_DEVICES; i++) {
    device *d = &devices[i];
    if (!d->set) {
      break;
    }
    if (d->mills) {
      int age_min = (cur_ms() - d->mills) / 60000;
      snprintf(buf, sizeof(buf), "%s %d%% %dm ago", d->device, d->battery, age_min);
    } else {
      snprintf(buf, sizeof(buf), "%s %d%%", d->device, d->battery);
    }
    
    tft.drawString(buf, 5, y, 2);
    y += 20;
  }
  always_draw_time();
  draw_mem();
}


unsigned long long last_check_devices_ms = 0;
unsigned long long last_check_entries_ms = 0;

int brightness = 100;
unsigned long last_touch_ms = 0;

void handleTouch(TS_Point p) {
  if (p.z < 2000) {
    return;
  }
  if (last_touch_ms == 0 || millis() - last_touch_ms > 200) {
    last_touch_ms = millis();
  } else {
    return;
  }
      snprintf(ota_status, sizeof(ota_status), "%u %u", p.x,p.y);
        ota_last_progress_ms = millis();
draw_header();
delay(500);
  if (p.x < 2000) {
    // right half
    if (p.y < 2000) {
      brightness -= 10;
    } else {
      brightness += 10;
    }
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    Serial.print("Setting brightness to ");
    Serial.println(brightness);
    ledcAnalogWrite(LEDC_CHANNEL_0, brightness);

      snprintf(ota_status, sizeof(ota_status), "Brightness: %u", brightness);
        ota_last_progress_ms = millis();
draw_header();
  } else {
    // left half
    if (p.y < 2000) {
      // top half
      // do something
    } else {
      // bottom half
      // do something
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  draw_header();
  maybe_draw_time();
  if (WiFi.status() == WL_CONNECTED) {
    
    ArduinoOTA.handle();
  } else {
    
    WiFi.reconnect();
    delay(1000);

    return;
    // wifiManager.autoConnect();
  }
  
  bool have_ns_update = false;
  if (last_check_devices_ms == 0 || millis() - last_check_devices_ms > devices_update_interval_ms)
  {
    have_ns_update |= update_nightscout_devices();
    last_check_devices_ms = millis();
  }
  if (last_check_entries_ms == 0 || millis() - last_check_entries_ms > entries_update_interval_ms)
  {
    have_ns_update |= update_nightscout_entries();
    last_check_entries_ms = millis();
  }
  if (have_ns_update) {
    draw_nightscout_data();
  }



  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    handleTouch(p);
  }
}
