#include "display.h"

#include <TFT_eSPI.h>

extern TFT_eSPI tft;


void draw_header(display_data_t *data, display_data_t *prev_data) {
  tft.setTextSize(1);
  tft.fillRect(0, 0, 320, 30, TFT_BLACK);
  if (data->wifi_connected != prev_data->wifi_connected) {
    if (data->wifi_connected) {
      tft.setTextColor(TFT_DARKGREEN, TFT_BLACK);
      tft.setTextDatum(TR_DATUM);
      int textWidth = tft.textWidth("Not connected", 2) - tft.textWidth("Connected", 2);
      tft.fillRect(320 - textWidth, 0, textWidth, 30, TFT_BLACK);
      tft.drawString("Connected", 320, 0, 2);
    } else {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setTextDatum(TR_DATUM);
      tft.drawString("Not connected", 320, 0, 2);  // this is wider so it will overwrite the previous "connected" string
      // don't hide the IP address if we had it before, for informational purposes
    }
  }

  if (strcmp(data->lan_ip, prev_data->lan_ip) || strcmp(data->status_override, prev_data->status_override)) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    if (data->status_override[0] == '\0') {
      if (prev_data->status_override[0] != '\0') {
        int ip_width = tft.textWidth(data->lan_ip, 2);
        int prev_override_width = tft.textWidth(prev_data->status_override, 2);
        if (prev_override_width > ip_width) {
          tft.fillRect(ip_width, 0, prev_override_width-ip_width, 30, TFT_BLACK);
        }
      } else {
        int ip_width = tft.textWidth(data->lan_ip, 2);
        int prev_ip_width = tft.textWidth(prev_data->lan_ip, 2);
        if (prev_ip_width > ip_width) {
          tft.fillRect(ip_width, 0, prev_ip_width-ip_width, 30, TFT_BLACK);
        }
      }
      tft.drawString(data->lan_ip, 0, 0, 2);
      tft.setTextDatum(TC_DATUM);
      tft.drawString("NightScout", 160, 0, 2);
    } else {
      if (prev_data->status_override[0] != '\0') {
        int override_width = tft.textWidth(data->status_override, 2);
        int prev_override_width = tft.textWidth(prev_data->status_override, 2);
        if (prev_override_width > override_width) {
          tft.fillRect(override_width, 0, prev_override_width-override_width, 30, TFT_BLACK);
        }
      } else {
        int override_width = tft.textWidth(data->status_override, 2);
        int prev_ip_width = tft.textWidth(prev_data->lan_ip, 2);
        if (prev_ip_width > override_width) {
          tft.fillRect(override_width, 0, prev_ip_width-override_width, 30, TFT_BLACK);
        }
      }
      tft.drawString(data->status_override, 0, 0, 2);
    }
  }
}


void maybe_draw_time() {

  // if (prev_draw_time_ms == 0 || (millis() - prev_draw_time_ms > 1000) && (cur_ms() / 1000) % 60 == 0) {
  //   always_draw_time();
  //   prev_draw_time_ms = millis();
  // }

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

void draw_time(display_data_t *data, display_data_t *prev_data) {
  // get current time in UTC
  time_t aTime = time(NULL); // get the time - this is GMT based.
  // convert this time to NYC time
  
}

void draw(display_data_t *data, display_data_t *prev_data) {
  draw_header(data, prev_data);
  draw_time(data, prev_data);
}