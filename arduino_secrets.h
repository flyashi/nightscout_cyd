#ifndef __ARDUINO_SECRETS_H__
#define __ARDINO_SECRETS_H__

// Define your WiFi credentials
#define SECRET_WIFI_SSID "your_ssid"
#define SECRET_WIFI_PSK "your_psk"

// of the form: https://your_nightscout_site.herokuapp.com/api/v1/entries/sgv?count=1
#define NIGHTSCOUT_ENTRIES_URL "https://your_nightscout_site/api/v1/entries/sgv?count=1"

// of the form: https://your_nightscout_site.herokuapp.com/api/v1/devicestatus/?count=20 (in case there's multiple devices)
#define NIGHTSCOUT_DEVICES_URL "https://your_nightscout_site/api/v1/devicestatus/?count=20"


#endif // __ARDUINO_SECRETS_H__