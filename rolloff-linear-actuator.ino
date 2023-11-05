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

#include <ESP8266WiFi.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

#include "Arduino_DebugUtils.h"

#define DEFAULT_LOG_LEVEL DBG_INFO

void setup_debug() {
  Debug.timestampOn();
  Debug.formatTimestampOn();
  Debug.newlineOn();
  Debug.setDebugLevel(DEFAULT_LOG_LEVEL);
}

/*
 * This version of the rolloff.ino has been modified to work with linear actuators that turn
 * themselves off when they reach their full extension and require the power to be 'inverted'
 * (+tive to -tive and -tive to +tive) so they can be retracted.
 * Review additions to the other Arduino samples to see if they are a more appropriate base.
 * The auxiliary button and light in the remote driver are to turn On or OFF the observatory lights.
 */

#define BAUD_RATE 115200  // USB connection baud rate

// Define USE_WIFI if WiFi is to be used instead of USB to communicate with INDI driver.
// If using WiFi and the serial monitor is to be used for diagnostic messaging in the IDE,
// the USB must also be connected between the Arduino and the host.
// If the USE_WIFI line is commented out then WiFi support is not built and the USB
// connection will be dedicated to communicating with the INDI driver.
int USE_WIFI = 1;

// If there is difficulty getting the INDI roof driver to connect, need to establish that the
// WiFi network is working and that the Arduino is connected. Running the Arduino IDE and
// using its serial monitor can be helpful to see any diagnostic messages.
// INIT_DELAY_SECS can be defined to wait a number of seconds after starting the Arduino
// to have time to get the serial monitor display started and see network startup messages.
#define INIT_DELAY_SECS 0

// For use by the WiFi example

#define OPEN_CONTACT HIGH  // Switch definition, Change to LOW if pull-down resistors are used.
// #define OPEN_CONTACT LOW  // Switch definition, Change to HIGH if pull-up resistors are used.

// Define name to pin assignments
#define SWITCH_1 5  // D1
#define SWITCH_2 4  // D2
//#define SWITCH_3 A2
//#define SWITCH_4 A3

#define RELAY_1 14  // D5: Motor A PWM
#define RELAY_2 12  // D6: Motor A direction
// #define RELAY_3 8
// #define RELAY_4 12
#define RELAY_5 13  // D7: Enable motor A

#define RELAY_10 0   // D3: Actuator Power GG
#define RELAY_11 2   // D4: Direction GG
#define RELAY_12 15  // D8: Enable motor B

#define MOTOR_ENABLE_A RELAY_5
#define MOTOR_ENABLE_B RELAY_12

// Indirection to define a functional name in terms of a switch
// Use 0 if switch not implemented
#define SWITCH_OPENED SWITCH_1  // Fully opened is assigned to switch 1
#define SWITCH_CLOSED SWITCH_2  // Fully closed is assigned to switch 2
#define SWITCH_LOCKED 0         // External lock
#define SWITCH_AUX 0            // Auxiliary switch

// Indirection to define a functional name in terms of a relay
// Use 0 if function not supportd
#define FUNC_ACTIVATION 100  // Activation relay connected to the direction relay GG
#define FUNC_STOP 200        // FUNC_STOP (abort) needs only to operate activation relay GG

#define FUNC_ACTIVATION_A RELAY_1   // Activation relay connected to the direction relay GG
#define FUNC_DIRECTION_A RELAY_2    // Direction relay inverts the power for either actuator extension or retraction GG
#define FUNC_ACTIVATION_B RELAY_10  // Activation relay connected to the direction relay GG
#define FUNC_DIRECTION_B RELAY_11   // Direction relay inverts the power for either actuator extension or retraction GG
// #define FUNC_LOCK 1              // If automated roof lock is available.
// #define FUNC_AUX 1          // Relay to turn ON or OFF observatory lights GG
// #define FUNC_BLINKER 0     // Relay to turn safety  on/off GG

/*
 * Abort (stop) request is only meaningful if roof is in motion.
 *
 * On Abort for a single button controller, only want to activate relay and pulse the controller if
 * the roof is still moving, then it would stop. If it has already reached the end of its
 * travel, a pulse could set it off again in the opposite direction.
 *
 * In case the end of run switches are not reached, some way to know if it is moving
 * would be helpful. Short of that estimate how long it takes the roof to open or close
 */
#define ROOF_OPEN_MILLI 60000

/*
 * GG
 * Delays required  1) before checking limit switches when the roof opens and
 *                  2) before turning the power off, after the limit switches are activated
 * May need to be adjusted
 */
#define ROOF_MOVEMENT_MIN_TIME_MILLIS 8000
#define ROOF_MOTION_END_DELAY_MILLIS 2000

// Buffer limits
#define MAX_INPUT 45
#define MAX_RESPONSE 127
#define MAX_MESSAGE 63

#define INTERNET_ADDR 192, 168, 1, 61  // Manual setup of IP address
#define GATEWAY_ADDR 192, 168, 1, 1    // Gateway address
#define SUBNET_ADDR 255, 255, 255, 0   // Subnet address
#define INTERNET_PORT 8888             // Listen on telnet port, match in INDI driver tab \

IPAddress ip(INTERNET_ADDR);           // AP local Internet address
IPAddress gw(GATEWAY_ADDR);            // AP gateway address
IPAddress subnet(SUBNET_ADDR);         // AP subnet address
WiFiServer server(INTERNET_PORT);      // Arduino server listening for connections on port specified
WiFiClient client;                     // Connection to return data back to the indi driver

enum cmd_input {
  CMD_NONE,
  CMD_OPEN,
  CMD_CLOSE,
  CMD_STOP,
  CMD_LOCK,
  CMD_AUXSET,
  CMD_CONNECT,
  CMD_DISCONNECT
} command_input;


unsigned long timeMove = 0;
unsigned long MotionStartTime = 0;  // Related to ROOF_MOVEMENT_MIN_TIME_MILLIS GG
unsigned long MotionEndDelay;       // Related to ROOF_MOTION_END_DELAY_MILLIS GG

const int cLen = 15;
const int tLen = 15;
const int vLen = MAX_RESPONSE;
char command[cLen + 1];
char target[tLen + 1];
char value[vLen + 1];

//  Maximum length of messages = 63                                               *|
const char* ERROR1 = "The controller response message was too long";
const char* ERROR2 = "The controller failure message was too long";
const char* ERROR3 = "Command input request is too long";
const char* ERROR4 = "Invalid command syntax, both start and end tokens missing";
const char* ERROR5 = "Invalid command syntax, no start token found";
const char* ERROR6 = "Invalid command syntax, no end token found";
const char* ERROR7 = "Roof controller unable to parse command";
const char* ERROR8 = "Command must map to either set a relay or get a switch";
const char* ERROR9 = "Request not implemented in controller";
const char* ERROR10 = "Abort command ignored, roof already stationary";

const char* VERSION_ID = "V1.3-esp-2ch-wifi-magnet-2";

void sendAck(char* val) {
  char response[MAX_RESPONSE];
  DEBUG_INFO("ACK=%s", val);  // DEBUG
  if (strlen(val) > MAX_MESSAGE) {
    strncpy(response, val, MAX_MESSAGE - 3);
    strcpy(&response[MAX_MESSAGE - 3], "...");
    sendNak(ERROR1);
    sendNak(response);
  } else {
    strcpy(response, "(ACK:");
    strcat(response, target);
    strcat(response, ":");
    strcat(response, val);
    strcat(response, ")");
    if (USE_WIFI == 1) {
      DEBUG_VERBOSE("about to send response: %s", response);  // DEBUG
      client.println(response);
      client.flush();
    } else {
      Serial.println(response);
      Serial.flush();
    }
  }
}

void sendNak(const char* errorMsg) {
  char buffer[MAX_RESPONSE];
  DEBUG_INFO("ACK=%s", errorMsg);  // DEBUG
  if (strlen(errorMsg) > MAX_MESSAGE) {
    strncpy(buffer, errorMsg, MAX_MESSAGE - 3);
    strcpy(&buffer[MAX_MESSAGE - 3], "...");
    sendNak(ERROR2);
    sendNak(buffer);
  } else {
    strcpy(buffer, "(NAK:ERROR:");
    strcat(buffer, value);
    strcat(buffer, ":");
    strcat(buffer, errorMsg);
    strcat(buffer, ")");
    if (USE_WIFI == 1) {
      client.println(buffer);
      // client.flush();
    } else {
      Serial.println(buffer);
      Serial.flush();
    }
  }
}


/*
 * Get switch value
 * NO contacts used and configured with a pull up resistor to the Arduino input pin. Logical 1 input when switch opened. GG
 * When switch closes The LOW voltage logical 0 is applied to the input pin. GG
 * The off or on value is to be sent to the host in the ACK response
 */
void getSwitch(int id, char* value) {
  if (digitalRead(id) == OPEN_CONTACT)
    strcpy(value, "OFF");
  else
    strcpy(value, "ON");
  DEBUG_DEBUG("gs:id=%d,v=%s", id, value);  // DEBUG
}

bool isSwitchOn(int id) {
  char switch_value[16 + 1];
  getSwitch(id, switch_value);
  if (strcmp(switch_value, "ON") == 0) {
    return true;
  }
  return false;
}

int read_data(char* inpBuf, int offset) {
  int recv_count = 0;
  if (USE_WIFI == 1) {
    if (client.available() > 0) {
      recv_count = client.read((unsigned char*)inpBuf + offset, 1);
      DEBUG_VERBOSE("Reading data: %d '%s'", recv_count, inpBuf);  // DEBUG
    } else {
      DEBUG_WARNING("read data no data available");  // DEBUG
    }
  } else {
    if (Serial.available() > 0) {
      Serial.setTimeout(1000);
      recv_count = Serial.readBytes((inpBuf + offset), 1);
    }
  }
  return recv_count;
}

bool parseCommand()  // (command:target:value)
{
  bool start = false;
  bool eof = false;
  int recv_count = 0;
  int wait = 0;
  int offset = 0;
  char startToken = '(';
  char endToken = ')';
  const int bLen = MAX_INPUT;
  char inpBuf[bLen + 1];

  memset(inpBuf, 0, sizeof(inpBuf));
  memset(command, 0, sizeof(command));
  memset(target, 0, sizeof(target));
  memset(value, 0, sizeof(value));

  while (!eof && (wait < 20)) {
    recv_count = read_data(inpBuf, offset);
    if (recv_count == 1) {
      offset++;
      if (offset >= MAX_INPUT) {
        sendNak(ERROR3);
        return false;
      }
      if (inpBuf[offset - 1] == startToken) {
        start = true;
      }
      if (inpBuf[offset - 1] == endToken) {
        eof = true;
        inpBuf[offset] = '\0';
      }
      continue;
    }
  }
  DEBUG_INFO("Received command=%s", inpBuf);  // DEBUG
  wait++;
  delay(100);

  if (!start || !eof) {
    if (!start && !eof) {
      sendNak(ERROR4);
    } else if (!start) {
      sendNak(ERROR5);
    } else if (!eof) {
      sendNak(ERROR6);
    }
    return false;
  } else {
    strcpy(command, strtok(inpBuf, "(:"));
    strcpy(target, strtok(NULL, ":"));
    strcpy(value, strtok(NULL, ")"));
    DEBUG_DEBUG("cmd=%s, t=%s, v=%s", command, target, value);  // DEBUG
    if ((strlen(command) >= 3) && (strlen(target) >= 1) && (strlen(value) >= 1)) {
      return true;
    } else {
      sendNak(ERROR7);
      return false;
    }
  }
}

void checkConnection() {
  runCommand(CMD_CONNECT, value);
  // sendAck(value);
}

bool is_data_available() {
  bool result = false;

  if (USE_WIFI == 1)
    result = (client && (client.available() > 0));
  else
    result = (Serial && (Serial.available() > 0));

  return result;
}
/*
 * Use the parseCommand routine to decode message
 * Determine associated action in the message. Resolve the relay or switch associated
 * pin with the target identity. Acknowledge any initial connection request. Return
 * negative acknowledgement with message for any errors found.  Dispatch to commandReceived
 * or requestReceived routines to activate the command or get the requested switch state
 */
void receiveCommand() {
  // Confirm there is input available, read and parse it.
  if (is_data_available()) {
    DEBUG_DEBUG("Data is available");  // DEBUG
    if (parseCommand()) {
      unsigned long timeNow = millis();
      int relay = -1;  // -1 = not found, 0 = not implemented, pin number = supported
      int sw = -1;     //      "                 "                    "
      bool connecting = false;
      const char* error = ERROR8;

      // On initial connection return the version
      if (strcmp(command, "CON") == 0) {
        connecting = true;
        strcpy(value, VERSION_ID);  // Can be seen on host to confirm what is running
        checkConnection();
      }

      // Map the general input command term to the local action
      // SET: OPEN, CLOSE, ABORT, LOCK, AUXSET
      else if (strcmp(command, "SET") == 0) {
        // Prepare to OPEN
        if (strcmp(target, "OPEN") == 0) {
          command_input = CMD_OPEN;
          relay = FUNC_ACTIVATION;
          timeMove = timeNow;
        }
        // Prepare to CLOSE
        else if (strcmp(target, "CLOSE") == 0) {
          command_input = CMD_CLOSE;
          relay = FUNC_ACTIVATION;
          timeMove = timeNow;
        }
        // Prepare to ABORT
        else if (strcmp(target, "ABORT") == 0) {
          command_input = CMD_STOP;

          // Test whether or not to Abort
          if (!isStopAllowed()) {
            error = ERROR10;
          } else {
            relay = FUNC_STOP;
          }
        }
        // Prepare for the Lock function
        /*
        else if (strcmp(target, "LOCK") == 0) {
          command_input = CMD_LOCK;
          relay = FUNC_LOCK;
        }

        // Prepare for the Auxiliary function
        else if (strcmp(target, "AUXSET") == 0) {
          command_input = CMD_AUXSET;
          relay = FUNC_AUX;
        }
        */
      }

      // Handle requests to obtain the status of switches
      // GET: OPENED, CLOSED, LOCKED, AUXSTATE
      else if (strcmp(command, "GET") == 0) {
        if (strcmp(target, "OPENED") == 0) {
          sw = SWITCH_OPENED;
        } else if (strcmp(target, "CLOSED") == 0) {
          sw = SWITCH_CLOSED;
        } else if (strcmp(target, "LOCKED") == 0) {
          sw = SWITCH_LOCKED;
        } else if (strcmp(target, "AUXSTATE") == 0) {
          sw = SWITCH_AUX;
        }
      }

      /*
       * See if there was a valid command or request
       */
      if (!connecting) {
        if ((relay == -1) && (sw == -1)) {
          sendNak(error);  // Unknown input or Abort command was rejected
        }

        // Command or Request not implemented
        else if ((relay == 0 || relay == -1) && (sw == 0 || sw == -1)) {
          DEBUG_ERROR("Command or request not implemented");  // DEBUG
          strcpy(value, "OFF");                          // Request Not implemented
          //sendNak(ERROR9);
          sendAck(value);
        }

        // Valid input received

        // A command was received
        // Set the relay associated with the command and send acknowlege to host
        else if (relay > 0)  // Set Relay response
        {
          runCommand(command_input, value);
        }

        // A state request was received
        else if (sw > 0)  // Get switch response
        {
          DEBUG_VERBOSE("about to get Status");  // DEBUG
          getStatus(sw);
        }
      }  // end !connecting
    }    // end command parsed
  }      // end input found
  else {
    DEBUG_WARNING("No data available. Continue...");  // DEBUG
  }
}


////////////////////////////////////////////////////////////////////////////////
// Abort movement command received, test to see if abort is allowed.
// If not return false and an error message will be returned to the host. If yes then return true.
// If either fully open or fully closed switches are on then deny the request by returning false.
// If neither switch is on then if there is a specific button (relay) assigned that can stop movement then return true
// to allow it to do so.
//
// This implementation assumes a one button setup and one which does not know if the roof is still moving or
// has hit something and already stopped. Before taking action see how long it has been since movement was initiated.
// If it is longer than the estimate to open or close the roof, assume motion has already stopped. In this case avoid
// emulating the single button push because that would set the roof moving again. If it seems that the roof
// could be moving then return true.
//
// Returning true will cause the Abort request to appear in the commandReceived routine where it will activate
// the requested relay.
//
bool isStopAllowed() {
  unsigned long timeNow = millis();

  // If the roof is either fully opened or fully closed, ignore the request.
  if (isSwitchOn(SWITCH_OPENED) || isSwitchOn(SWITCH_CLOSED)) {
    return false;
  }

  // If time since last open or close request is longer than the time for the roof travel return false
  if ((timeNow - timeMove) >= ROOF_OPEN_MILLI) {
    return false;
  } else

  // Stop will be attempted
  {
    return true;
  }
}

/*
 * Commands
 *
*/

void motor_off() {
  // Disable current to motors
  digitalWrite(MOTOR_ENABLE_A, LOW);
  digitalWrite(MOTOR_ENABLE_B, LOW);

  // Make sure motors are stopped
  digitalWrite(FUNC_DIRECTION_A, LOW);   // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, LOW);   // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion
}

void motor_on() {
  // Make sure motors are stopped
  digitalWrite(FUNC_DIRECTION_A, LOW);   // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, LOW);   // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion

  // Enable current to motors
  digitalWrite(MOTOR_ENABLE_A, HIGH);
  digitalWrite(MOTOR_ENABLE_B, HIGH);
}

void stopCommand() {
  motor_off();  // Disable the motor
  // digitalWrite(FUNC_BLINKER, LOW);
}

void connectCommand() {
  stopCommand();
}

void openCommand() {
  // digitalWrite(FUNC_BLINKER, HIGH);  // Blink when opening roof

  motor_on();                             // Activate the motor
  digitalWrite(FUNC_DIRECTION_A, LOW);    // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, HIGH);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, LOW);    // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, HIGH);  // Set actuator in motion

  MotionStartTime = millis();
}

void closeCommand() {
  // digitalWrite(FUNC_BLINKER, HIGH);  // Blink when closing roof

  motor_on();                            // Activate the motor
  digitalWrite(FUNC_DIRECTION_A, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_A, LOW);  // Set actuator in motion

  digitalWrite(FUNC_DIRECTION_B, HIGH);  // Set actuator voltage leads to open actuator
  digitalWrite(FUNC_ACTIVATION_B, LOW);  // Set actuator in motion

  MotionStartTime = millis();
}


////////////////////////////////////////////////////////////////////////////////
// Action command received

// Here after pin associations resolved and request action known
// Default action is to set the associated relay to the requested state "ON" or "OFF" and
// send acknowledgement to the host.
// target is the name associated with the relay "OPEN", "CLOSE", "STOP", "LOCK", "AUXSET".
// It will be used when  sending the acknowledgement to the host. Find out if a particular
// command is being processed using if (strcmp(target, "OPEN") == 0) {do something}
//
// relay: pin id of the relay
// value: How to set the relay "ON" or "OFF"
//
//
void runCommand(int command_input, char* value) {
  DEBUG_DEBUG("runCommand %d, %s", command_input, value); // DEBUG

  // Stop
  if (command_input == CMD_STOP) {

    stopCommand();

  } else  // Resume Parsing

    // Connect
    if (command_input == CMD_CONNECT) {

      connectCommand();

    } else  // Resume Parsing
            /*
      // AUX Set
      if (command_input == CMD_AUXSET) {

        if (strncmp(value, "ON", 2)) {
          digitalWrite(FUNC_AUX, LOW);
        }
        if (strncmp(value, "OFF", 3)) {
          digitalWrite(FUNC_AUX, HIGH);
        }

      } else  // Resume Parsing
      */
      // Open
      if (command_input == CMD_OPEN) {

        openCommand();
      } else  // Resume Parsing

        // Close
        if (command_input == CMD_CLOSE) {
          closeCommand();
        }

  sendAck(value);  // Send acknowledgement that relay pin associated with "target" was activated to value requested
}

////////////////////////////////////////////////////////////////////////////////

// Here after pin associations resolved and request action known
// Request to obtain state of a switch
// Default action is to read the associated switch and return result to the host
// target is the name associated with the switch "OPENED", "CLOSED" etc and will
// be used when sending the acknowledgement to the host. Find out if a certain request is being processed using
// if (strcmp(target, "OPENED") == 0) {do something}
//
// sw:     The switch's pin identifier.
// value   getSwitch will read the pin and set this to "ON" or "OFF"
void getStatus(int sw) {
  getSwitch(sw, value);
  sendAck(value);  // Send result of reading pin associated with "target"
}

void connectWifi() {
  // Connect to the WiFi network:
  while (WiFi.status() != WL_CONNECTED) {
    DEBUG_INFO("Attempting to connect to the network ");

    // wait up to 20 seconds to establish the connection
    for (int i = 0; i < 20; i++) {
      delay(1000);
      if (WiFi.status() == WL_CONNECTED)
        return;
      DEBUG_INFO(".");
    }
    DEBUG_INFO("Failed to connect to the current configured network.\n" 
               "Unable to continue without a WiFi network.");
  }
}

void printWifiStatus() {
  String ssid = WiFi.SSID();
  uint8_t rssi = WiFi.RSSI();
  IPAddress ip = WiFi.localIP();
  
  // print the SSID of the network you're attached to:
  DEBUG_INFO("\nSSID: %s, IP Address: %s, Signal (RSSI): %d dBm", ssid.c_str(), ip.toString().c_str(), rssi);
}

// Check if roof has fully opened or fully closed and turn off relay if so! GG

void check_roof_turn_off_relays() {

  if (MotionEndDelay == 0) {
    if (MotionStartTime != 0) {
      if ((millis() - MotionStartTime) > ROOF_MOVEMENT_MIN_TIME_MILLIS) {
        if (isSwitchOn(SWITCH_OPENED) || isSwitchOn(SWITCH_CLOSED)) {
          DEBUG_DEBUG("Roof is not moving...");  // DEBUG
          MotionEndDelay = millis();
        }
      }
    }
  } else {  // Add some delay for complete roof opening or closure
    if ((millis() - MotionEndDelay) > ROOF_MOTION_END_DELAY_MILLIS) {
      DEBUG_DEBUG("Stop...");  // DEBUG
      stopCommand();
      MotionEndDelay = 0;
    }
    MotionStartTime = 0;
  }
}


/*
 * One time initialization
 */
void setup() {
  // Wait for all ports to stabilize
  delay(100);

  // Initialize the input switches
  pinMode(SWITCH_1, INPUT_PULLUP);
  pinMode(SWITCH_2, INPUT_PULLUP);
  // pinMode(SWITCH_3, INPUT_PULLUP);
  // pinMode(SWITCH_4, INPUT_PULLUP);

  // Initialize the relays
  //Pin Setups
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  // pinMode(RELAY_3, OUTPUT);
  // pinMode(RELAY_4, OUTPUT);
  pinMode(RELAY_5, OUTPUT);

  pinMode(RELAY_10, OUTPUT);
  pinMode(RELAY_11, OUTPUT);
  pinMode(RELAY_12, OUTPUT);

  setup_debug();

  // Establish USB port.
  setup_serial();

  if (USE_WIFI == 1) {
    setup_wifi();
  }

  // Turn Off the relays.
  motor_off();

  // Start server
  server.begin();
}

void setup_serial() {
  Serial.begin(BAUD_RATE);  // Baud rate to match that in the driver
  while (!Serial)
    ;
}

void setup_wifi() {
  delay(INIT_DELAY_SECS * 1000);  // diagnostic, allow time to get serial monitor displayed
  WiFi.mode(WIFI_STA);            // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("AutoConnectAP", "astroberry");  // password protected ap

  if (!res) {
    DEBUG_ERROR("Failed to connect");
    // ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    DEBUG_INFO("connected...yeey :)");
  }
  printWifiStatus();
  DEBUG_INFO("Network online, ready for rolloffino driver connections.");
}



/*
 *Wait here for command or switch request from host
 */

boolean indiConnected = false;  // Driver has connected to local network
// boolean indiData = false;       // Driver has made initial contact

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
  WiFi.disconnect();
  WiFi.config(ip, gw, subnet);  // Use a fixed WiFi address for the Arduino
  connectWifi();
  server.begin();  // Start listening
  printWifiStatus();
}


void wifi_loop() {
  // Check still connected to the wifi network
  DEBUG_VERBOSE("wifi loop");  // DEBUG
  int wifi_status = WiFi.status();
  DEBUG_VERBOSE("wifi status: %d", wifi_status);  // DEBUG
  if (wifi_status != WL_CONNECTED) {
    DEBUG_VERBOSE("reconnecting...");  // DEBUG
    reconnectWifi();
  }

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

  // Wait for incoming data from the INDI driver
  if (client) {
    client.flush();
    /*
    if (!indiData) {
      client.flush();
      indiData = true;
    }
    */
    DEBUG_VERBOSE("available data...");  // DEBUG
    if (client.available() > 0) {
      receiveCommand();
    }
  } else {
    DEBUG_VERBOSE("No data available. Sleeping...");  // DEBUG
    delay(100);
  }
}

void serial_loop() {
  while (Serial.available() <= 0) {
    for (int cnt = 0; cnt < 60; cnt++) {
      if (Serial.available() > 0)
        break;
      else
        delay(100);
    }
  }
  receiveCommand();  // Some input detected
}

/*
Command Format:  (command:target|state:value)
Response Format: (response:target|state:value)

Command:
       CON 	(CON:0:0)              Establish initial connection with Arduino
       GET	(GET:state:value)      Get state of a switch
       SET 	(SET:target:value)     Set relay closed or open

           state:   OPENED | CLOSED | LOCKED | AUXSTATE
           target:  OPEN | CLOSE | ABORT | LOCK | AUXSET
           value:   ON | OFF | 0 | text-message

Response:
       ACK       Success returned from Arduino
       NAK       Failure returned from Arduino

Examples:        From the Driver      Response From the Arduino
                 ----------------     --------------------------
Initial connect (CON:0:0)          >
                                   <  (ACK:0:version) | (NAK:ERROR:message)
Read a switch   (GET:OPENED:0)     >
                                   <  (ACK:OPENED:ON|OFF) | (NAK:ERROR:message)
Set a relay     (SET:CLOSE:ON|OFF) >
                                   <  (ACK:CLOSE:ON|OFF) | (NAK:ERROR:message)
*/

// Wait here for command or switch request from host
void loop() {
  check_roof_turn_off_relays();  // GG

  if (USE_WIFI == 1) {
    wifi_loop();
  } else {
    serial_loop();
  }
}  // end routine loop
