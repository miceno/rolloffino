#ifndef __wifi_secrets_h__
#define __wifi_secrets_h__

#define WIFI_DEFAULT_AP_SSID "MY-SSID"
#define WIFI_DEFAULT_AP_SECRET "MY-WIFI-SECRET"

struct WifiCredentials {
    const char* ssid;
    const char* password;
};

#define WIFI_SSID "SSID_1"
#define WIFI_SSID_PASSWORD "MYFAVOURITEPASSWORD"

#define WIFI_SSID2 "SSID_2"
#define WIFI_SSID_PASSWORD2 "MYFAVOURITEPASSWORD"


static const WifiCredentials wifi_list[] = {
    { WIFI_SSID, WIFI_SSID_PASSWORD },
    { WIFI_SSID2, WIFI_SSID_PASSWORD2 }
};

static int wifi_list_size = sizeof(wifi_list) / sizeof(wifi_list[0]);

#endif