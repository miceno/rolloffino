
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
