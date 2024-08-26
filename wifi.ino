// #ifndef __rolloff_linear_actuator_wifi__
// #define __rolloff_linear_actuator_wifi__
#include <ESP8266mDNS.h>
#include <DoubleResetDetector.h>

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 5

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);


void connectWifi() {
  // Connect to the WiFi network:
  while (WiFi.status() != WL_CONNECTED) {
    DEBUG_INFO("Attempting to connect to network '%s'", WiFi.SSID());

    // wait to establish the connection
    for (int i = 0; i < WIFI_MAX_RETRIES; i++) {
      delay(1000);
      WiFi.reconnect();
      if (WiFi.status() == WL_CONNECTED)
        return;
      DEBUG_INFO(".");
    }
    DEBUG_INFO("Failed to connect to '%s' network.\n"
               "Unable to continue without a WiFi network. Restarting...",
               WiFi.SSID());

    restart();
  }
}

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


void setup_wifi() {
  delay(INIT_DELAY_SECS * 1000);  // diagnostic, allow time to get serial monitor displayed
  WiFi.mode(WIFI_STA);            // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  wm.setHostname("rolloffino");
  MDNS.begin("rolloffino");

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  //wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  if (drd.detectDoubleReset()) {

    //reset settings - for testing
    //wm.resetSettings();

    // set configportal timeout
    wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);

    Serial.printf("Double Reset detected. Starting configuration portal on %s...\n", WIFI_DEFAULT_AP_SSID);
    if (!wm.startConfigPortal(WIFI_DEFAULT_AP_SSID, WIFI_DEFAULT_AP_SECRET)) {
      Serial.println("failed to connect and hit timeout");
      restart();
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

  } else {
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    wm.setWiFiAutoReconnect(true);
    wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
    wm.setConnectTimeout(WIFI_CONNECTION_TIMEOUT);
    res = wm.autoConnect(WIFI_DEFAULT_AP_SSID, WIFI_DEFAULT_AP_SECRET);  // password protected ap

    if (!res) {
      DEBUG_ERROR("Failed to connect to SSID %s... restarting in %ds", wm.getWiFiSSID().c_str(), RESTART_DELAY);
      restart();
    } else {
      //if you get here you have connected to the WiFi
      DEBUG_INFO("connected to %s yeey :)", wm.getWiFiSSID().c_str());
    }
    printWifiStatus();
    DEBUG_INFO("Network online, ready for rolloffino driver connections.");
  }
}

void reconnectWifi() {
  DEBUG_VERBOSE("not connected");  // DEBUG
  // if (indiConnected || indiData)
  if (indiConnected) {
    DEBUG_INFO("Lost the WiFi connection");
  }
  indiConnected = false;
  if (client) {
    client.stop();
  }
  // WiFi.disconnect();
  // WiFi.config(ip, gw, subnet);  // Use a fixed WiFi address for the Arduino
  connectWifi();
  server.begin();  // Start listening
  printWifiStatus();
}

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

  reconnect_wifi_helper();

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

void drd_loop(){
  drd.loop();
}


// #endif