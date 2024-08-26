/*
 * A controller for linear actuator roof motor controller from the INDI rolloffino roof driver.
 *
 * tg August 2018  Original
 * tg February 2020 Generalize to make less installation specific
 *                  Add unspecified usage AUX switch and relay
 *                  Communication protocol in terms of function not switches/relays
 *                  ~ 15KB 2% of Due, 50% of Nano
 * tg November 2021 Break out commandReceived and requestReceived to make alernate actions more
 *                  obvious/accessible, Remove Due specific code.
 * gg September 2022 Modifications to control a linear actuator based roof
 * or September 2023 Refactor for modularity, for ESP8266 and TCP connection, based on rolloffino.ino.wifi
 *
 * tg: Tom Gibson
 * gg: Gilles Gagnon
 * or: Orestes Sanchez
 * tg January 2022  Add this example of using WiFi between the Arduino and the INDI rolloffino roof
 *                  driver
 */
/*
 * This version of the rolloff.ino has been modified to work with linear actuators that turn
 * themselves off when they reach their full extension and require the power to be 'inverted'
 * (+tive to -tive and -tive to +tive) so they can be retracted.
 * Review additions to the other Arduino samples to see if they are a more appropriate base.
 * The auxiliary button and light in the remote driver are to turn On or OFF the observatory lights.
 */


#include <ESP8266WiFi.h>
#include "Arduino_DebugUtils.h"

#include "config.h"
#include "motor.h"
#include "functions.h"

const char* VERSION_ID = "V1.4-esp-2ch-wifi-magnet-3";
//  Maximum length of messages = 63                                                       *|
static const char* ERROR1 = "The controller response message was too long";
static const char* ERROR2 = "The controller failure message was too long";
static const char* ERROR3 = "Command input request is too long";
static const char* ERROR4 = "Invalid command syntax, both start and end tokens missing";
static const char* ERROR5 = "Invalid command syntax, no start token found";
static const char* ERROR6 = "Invalid command syntax, no end token found";
static const char* ERROR7 = "Roof controller unable to parse command";
static const char* ERROR8 = "Command must map to either set a relay or get a switch";
static const char* ERROR9 = "Request not implemented in controller";
static const char* ERROR10 = "Abort command ignored, roof already stationary";


cmd_input command_input;

const int cLen = 15;
const int tLen = 15;
const int vLen = MAX_RESPONSE;
char command[cLen + 1];
char target[tLen + 1];
char value[vLen + 1];

unsigned long MotionStartTime = 0;  // Related to ROOF_MOVEMENT_MIN_TIME_MILLIS GG
unsigned long MotionStopTime;       // Related to ROOF_MOTION_END_DELAY_MILLIS GG

Motor* motor = new TA6586();


IPAddress ip(INTERNET_ADDR);       // AP local Internet address
IPAddress gw(GATEWAY_ADDR);        // AP gateway address
IPAddress subnet(SUBNET_ADDR);     // AP subnet address
WiFiServer server(INTERNET_PORT);  // Arduino server listening for connections on port specified
WiFiClient client;                 // Connection to return data back to the indi driver

boolean indiConnected = false;      // Driver has connected to local network
// boolean indiData = false;        // Driver has made initial contact


/*
 * One time initialization
 */
void setup() {
  // Establish USB port.
  setup_serial();

  // Initialize the input switches
  pinMode(SWITCH_1, INPUT_PULLUP);
  pinMode(SWITCH_2, INPUT_PULLUP);
  // pinMode(SWITCH_3, INPUT_PULLUP);
  // pinMode(SWITCH_4, INPUT_PULLUP);

  // Initialize the relays
  //Pin Setups
  pinMode(RELAY_A1, OUTPUT);
  pinMode(RELAY_A2, OUTPUT);
  // pinMode(RELAY_3, OUTPUT);
  // pinMode(RELAY_4, OUTPUT);

  pinMode(RELAY_B1, OUTPUT);
  pinMode(RELAY_B2, OUTPUT);

  setup_debug();

  if (USE_WIFI == 1) {
    setup_wifi();
  }

  // Turn Off the relays.
  motor->motorOff();

  // Start server
  server.begin();
}


// Wait here for command or switch request from host
void loop() {
  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd_loop();
  motor->checkRoofMovement();  // GG

  if (USE_WIFI == 1) {
    wifi_loop(motor);
  } else {
    serial_loop(motor);
  }
  delay(1);
}  // end routine loop
