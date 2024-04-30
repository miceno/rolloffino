#ifndef __rolloff_linear_actuator_config_h__
#define __rolloff_linear_actuator_config_h__ 1


#define DEFAULT_LOG_LEVEL DBG_INFO

#define BAUD_RATE 115200  // USB connection baud rate

// Define USE_WIFI if WiFi is to be used instead of USB to communicate with INDI driver.
// If using WiFi and the serial monitor is to be used for diagnostic messaging in the IDE,
// the USB must also be connected between the Arduino and the host.
// If the USE_WIFI line is commented out then WiFi support is not built and the USB
// connection will be dedicated to communicating with the INDI driver.
#define USE_WIFI 1

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

#define RELAY_10 0   // D3: Motor B PWM
#define RELAY_11 2   // D4: Motor B direction
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
#define FUNC_LOCK 0              // If automated roof lock is available.
#define FUNC_AUX 0          // Relay to turn ON or OFF observatory lights GG
#define FUNC_BLINKER 0     // Relay to turn safety  on/off GG

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
#define ROOF_MOVEMENT_MIN_TIME_MILLIS 20000
#define ROOF_MOTION_END_DELAY_MILLIS 5000

// Buffer limits
#define MAX_INPUT 45
#define MAX_RESPONSE 127
#define MAX_MESSAGE 63

#define INTERNET_ADDR 192, 168, 1, 61  // Manual setup of IP address
#define GATEWAY_ADDR 192, 168, 1, 1    // Gateway address
#define SUBNET_ADDR 255, 255, 255, 0   // Subnet address
#define INTERNET_PORT 8888             // Listen on telnet port, match in INDI driver tab

#endif