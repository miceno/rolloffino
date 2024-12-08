#include <ESP8266mDNS.h>
#include <DoubleResetDetector.h>
#include <WiFiManager.h>

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 5

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
WiFiManager wm;
unsigned long wifiPortalStartTime = millis();


void restart(void) {
  delay(RESTART_DELAY * 1000);
  ESP.restart();
}

void printWifiStatus() {
  String ssid = WiFi.SSID();
  int8_t rssi = WiFi.RSSI();
  IPAddress ip = WiFi.localIP();

  // print the SSID of the network you're attached to:
  DEBUG_INFO("SSID: %s, IP Address: %s, Signal (RSSI): %d dBm", ssid.c_str(), ip.toString().c_str(), rssi);
}

void startConfigPortal(void) {
  // Start configuration portal non blocking, so we can use rolloffino anyway.
  wm.setConfigPortalBlocking(false);

  DEBUG_INFO("Starting configuration portal with SSID %s (timeout %d seconds).", WIFI_DEFAULT_AP_SSID, WIFI_PORTAL_TIMEOUT);

  if (!wm.startConfigPortal(WIFI_DEFAULT_AP_SSID, WIFI_DEFAULT_AP_SECRET)) {
    DEBUG_DEBUG("Portal is already running");
  }
  wifiPortalStartTime = millis();
}

void setup_wifi() {
  // diagnostic, allow time to get serial monitor displayed
  delay(INIT_DELAY_SECS * 1000);
  // it is a good practice to make sure your code sets wifi mode how you want it.
  // explicitly set mode, esp defaults to STA+AP
  WiFi.mode(WIFI_STA);
  WiFi.forceSleepWake();
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  wm.setHostname("rolloffino");
  MDNS.begin("rolloffino");
  wm.setConfigPortalBlocking(false);
  // set configportal timeout
  // wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);

  bool res;
  if (drd.detectDoubleReset()) {
    DEBUG_INFO("Double Reset detected. Starting configuration portal on %s...", WIFI_DEFAULT_AP_SSID);

    startConfigPortal();
  } else {
    DEBUG_DEBUG("Connecting to last configured AP");

    // wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
    wm.setWiFiAutoReconnect(true);
    wm.setConnectTimeout(WIFI_CONNECTION_TIMEOUT);
    // password protected ap
    res = wm.autoConnect(WIFI_DEFAULT_AP_SSID, WIFI_DEFAULT_AP_SECRET);

    if (!res) {
      DEBUG_ERROR("Failed to connect to SSID %s... starting WM portal.", wm.getWiFiSSID().c_str());      // restart();
      startConfigPortal();
    } else {
      //if you get here you have connected to the WiFi
      DEBUG_INFO("connected to %s yeey :)", wm.getWiFiSSID().c_str());
      connectWifi();
    }
  }
  DEBUG_INFO("Network online, ready for rolloffino driver connections.");
}


void connectWifi() {
  DEBUG_VERBOSE("not connected");
  if (indiConnected) {
    DEBUG_INFO("Lost the WiFi connection");
  }
  indiConnected = false;
  if (client) {
    client.stop();
  }
  // Start listening
  server.begin();
  printWifiStatus();
}

WiFiClient get_wifi_client(WiFiClient client) {
  if (!client) {
    client = server.available();
  }
  if (client.connected()) {
    DEBUG_VERBOSE("client.connected");
    if (!indiConnected) {
      indiConnected = true;
      // indiData = false;
      DEBUG_INFO("rolloffino driver connected");
    }
  } else {
    DEBUG_VERBOSE("NOT client.connected");
    if (indiConnected) {
      indiConnected = false;
      DEBUG_INFO("rolloffino driver disconnected");
    }
  }
  DEBUG_VERBOSE("after client.connected checks");

  return client;
}


void wifi_loop() {
  MDNS.update();
  MDNS.addService("rolloffino", "tcp", 8888);

  // Process WiFiManager config portal
  wm.process();
  // check for timeout
  if(TimePeriodIsOver(wifiPortalStartTime, MILLIS(WIFI_PORTAL_TIMEOUT))){
  // if ((millis() - wifiPortalStartTime) > (WIFI_PORTAL_TIMEOUT * 1000)) {
    DEBUG_INFO("Portal timeout after %d seconds...", WIFI_PORTAL_TIMEOUT);
    if (wm.getConfigPortalActive()) {
      DEBUG_INFO("Config portal is active... restarting...");
      restart();
    } else {
      DEBUG_INFO("Config portal is not active... continue...");
      wifiPortalStartTime = millis();
    }
  }

}

void drd_loop() {
  drd.loop();
}

