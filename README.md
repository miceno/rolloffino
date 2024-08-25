
This project is based on the original [rolloffino code from INDI](https://github.com/indilib/indi-3rdparty/tree/master/indi-rolloffino). 

It only includes the relevant Arduino code, not the driver.

# Description

This is an implementation of the microcontroller code for the Rolloffino INDIlib driver.

The driver communicates with an Arduino microcontroller using a USB or WiFi connection. 
The driver sends open or close commands and requests from the Arduino whether the roof is opened or closed. 
The driver does not know how the roof is being operated.

[More information is in the documentation file.](doc/rolloffino.md)

# indilib driver
```
Driver:       RollOff ino
Executable:   indi_rolloffino
Minimum INDI: 2.0.1
```

# Logging setup

Console is running on the serial port, at 115200. The default log level is DBG_INFO, change it in the code with 
constant `DEFAULT_LOG_LEVEL`.

# Usage

The microcontroller code allows receiving commands by WiFi or by Serial. A static flag `USE_WIFI` controls the current mode.
Set it to `1` to use WiFi, any other value to use serial communication.

Sending command by serial and the debug console are not compatible.

# WiFi setup

Once in WiFi mode, the microcontroller tries to connect to a WiFi Access Point on boot. 

The very first time there is not such configured AP, so it fails to connect and it fallbacks to start its own 
Access Point: `AProlloffino`, see code for the default password.

Connect to `AProlloffino` to setup the AP to connect to on boot. Then reboot the microcontroller.

Here it is a sample of the console output on boot once an access point is configured, it show the device IP:
```
*wm:Connecting to SAVED AP: AP_123478
*wm:connectTimeout not set, ESP waitForConnectResult... 
*wm:AutoConnect: SUCCESS 
*wm:STA IP Address: 192.168.1.46
[ 00:00:01.525 ] connected...yeey :)
[ 00:00:01.526 ] 
SSID: AP_123478, IP Address: 192.168.1.46, Signal (RSSI): -58 dBm
[ 00:00:01.530 ] Network online, ready for rolloffino driver connections.
```

The microcontroller listens on port `8888` by default.


# Socket client

To test the communication with the microcontroller, use the supplied `utils/socket-client.py` script.

```shell
$ python3 socket-client.py --help
usage: socket-client.py [-h] [-s SERVER] [-n] [-p PORT]

Send and receive TCP messages over a persistent server connection.

optional arguments:
  -h, --help            show this help message and exit
  -s SERVER, --server SERVER
                        Server host
  -n, --newline         Add a new line to every request
  -p PORT, --port PORT  Server port
```

To use the socket client run:
```shell
python3 socket-client.py -p 8888 -s 192.168.1.46
```

Commands are one per line, delimited by parenthesis, like:
```
(CON:0:0)
```

# nc 

You can also use `nc` to send commands. You will need to remove end-of-line delimiter and wait for the response, like this:
```
(echo -n "(CON:0:0)" && sleep 1 && echo -n "(SET:OPEN:0)" && sleep 1) | nc 192.168.1.46 8888
```

# Set up

## Pinout for sensors

The microcontroller senses the begin and end position of the roof, by using a couple of sensor. 
The main sensor used is a magnetic switch.

Use the following pinout (from ESP8266 to switches), and set the corresponding value on the `config.h` file:

* D1 (GPIO5)  -> sensor OPENED NC    -> SWITCH_1
* G           -> sensor OPENED COMM
* D2 (GPIO4)  -> sensor CLOSED NC    -> SWITCH_2
* G           -> sensor CLOSED COMM

## Pinout for TA6586

TA6586 is a 2-channel motor controller.

Use the following pinout (from ESP8266 to motor board), and set the corresponding value on the `config.h` file:
* D3 (GPIO0)  -> D0 motor A      -> RELAY_A1
* D4 (GPIO2)  -> D1 motor A      -> RELAY_A2
* D5 (GPIO14) -> D2 motor B      -> RELAY_B1
* D6 (GPIO12) -> D3 motor B      -> RELAY_B2
* D7 (GPIO13) -> D2 motor B      -> RELAY_B1
* D8 (GPIO15) -> D3 motor B      -> RELAY_B2

## Pinout for DRV8871

DRV8871 is a 1-channel motor controller, so the pinout is per motor board:

Use the following pinout (from ESP8266 to motor board), and set the corresponding value on the `config.h` file:
* D3 (GPIO0)  -> IN1 motor A      -> RELAY_A1
* D4 (GPIO2)  -> IN2 motor A      -> RELAY_A2
* D5 (GPIO14) -> IN1 motor B      -> RELAY_B1
* D6 (GPIO12) -> IN2 motor B      -> RELAY_B2
* D7 (GPIO13) -> IN1 motor B      -> RELAY_B1
* D8 (GPIO15) -> IN2 motor B      -> RELAY_B2

## Stop motor timeout

Once the motor starts moving, it will continue indefinitely. Some linear actuators have limit sensors, so the motion will stop
but the motor will still be powered. In order to reduce power, the microcontroller stops the motors after a timeout.

It might be your linear actuators are faster or slower, so adjust the value for `ROOF_MOVEMENT_MIN_TIME_MILLIS` accordingly in 
`config.h`.

## Motor speed

The code drives two motors, but they might have different speeds. In the case of linear actuators, they might have
small differences in construction that will make one of them faster than the other.

This is the procedure to measure and adjust the difference in speeds:
* Measure the time it takes both motors to open and close separately. Take several measures and calculate the average
to have a valid metric. Say that when opening, motor A takes an average of 16s and motor B takes 18s.
* Calculate the percentage of speed of the faster: take the difference between the average of the slowest and fastest motor, 
and divide it by the average time of the fastest motor. For our example, the difference is 2s and the percentage is 12.5%.
* Reduce the speed of the fastest motor: Set an appropiate percentage for faster motor 
using variables `MOTOR_*_SPEED_FACTOR_*ING`. For our example, set `MOTOR_A_SPEED_FACTOR_OPENING` to 87.
* Set to 100 the values for the slowest motor.

You will probably need to manually adjust the value, since measuring times might not be accurate enough.
