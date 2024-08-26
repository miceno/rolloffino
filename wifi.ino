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
unsigned int startTime = millis();


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
  startTime = millis();
}

void setup_wifi() {
  delay(INIT_DELAY_SECS * 1000);  // diagnostic, allow time to get serial monitor displayed
  // it is a good practice to make sure your code sets wifi mode how you want it.
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
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
    res = wm.autoConnect(WIFI_DEFAULT_AP_SSID, WIFI_DEFAULT_AP_SECRET);  // password protected ap

    if (!res) {
      DEBUG_ERROR("Failed to connect to SSID %s... restarting in %ds", wm.getWiFiSSID().c_str(), RESTART_DELAY);
      // restart();
      startConfigPortal();
    } else {
      //if you get here you have connected to the WiFi
      DEBUG_INFO("connected to %s yeey :)", wm.getWiFiSSID().c_str());
      connectWifi();
    }
    DEBUG_INFO("Network online, ready for rolloffino driver connections.");
  }
}


void connectWifi() {
  DEBUG_VERBOSE("not connected");  // DEBUG
  // if (indiConnected || indiData)
  if (indiConnected) {
    DEBUG_INFO("Lost the WiFi connection");
  }
  indiConnected = false;
  if (client) {
    client.stop();
  }
  server.begin();  // Start listening
  printWifiStatus();
}
/*
void reconnect_wifi_helper() {
  // Check still connected to the wifi network
  DEBUG_VERBOSE("wifi loop");  // DEBUG
  int wifi_status = WiFi.status();
  DEBUG_VERBOSE("wifi status: %d", wifi_status);  // DEBUG
  if (wifi_status != WL_CONNECTED) {
    DEBUG_VERBOSE("reconnecting...");  // DEBUG
    reconnectWifi();
  }
}
*/
WiFiClient get_wifi_client(WiFiClient client) {
  if (!client) {
    client = server.available();
  }
  if (client.connected()) {
    DEBUG_VERBOSE("client.connected");  // DEBUG
    if (!indiConnected) {
      indiConnected = true;
      // indiData = false;
      DEBUG_INFO("rolloffino driver connected");
    }
  } else {
    DEBUG_VERBOSE("NOT client.connected");  // DEBUG
    if (indiConnected) {
      indiConnected = false;
      DEBUG_INFO("rolloffino driver disconnected");
    }
  }
  DEBUG_VERBOSE("after client.connected checks");  // DEBUG

  return client;
}

void wifi_loop(Motor *m) {
  MDNS.update();
  MDNS.addService("rolloffino", "tcp", 8888);

  // Process WiFiManager config portal
  wm.process();
  // check for timeout
  if ((millis() - startTime) > (WIFI_PORTAL_TIMEOUT * 1000)) {
    DEBUG_INFO("Portal timeout after %d seconds...", WIFI_PORTAL_TIMEOUT);
    if (wm.getConfigPortalActive()) {
      DEBUG_INFO("Config portal is active... restarting...");
      restart();
    }else{
      DEBUG_INFO("Config portal is not active... continue...");
      startTime = millis();
    }
  }

  client = get_wifi_client(client);
  // Wait for incoming data from the INDI driver
  if (client) {
    client.flush();
    if (client.available() > 0) {
      DEBUG_VERBOSE("available data...");  // DEBUG
      parseCommand(m);
    }
  } else {
    DEBUG_VERBOSE("No data available. Sleeping...");  // DEBUG
  }
}

void drd_loop() {
  drd.loop();
}


// #endif