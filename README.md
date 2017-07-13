# pi2mqtt

This tool monitors sensors connected to the Raspberry Pi and sends the data to a
broker via mqtt protocol.  The current sensors include:

## Temp -  DS18B20
You can connect multiple DS18B20 sensors to pin 4 of the rpi.  Be sure to include 
a 4.7Kohm pull up.  The tools will scan for devices in the `/sys/bus/w1/devices` directory.  DS18B20 devices will have the prefix of _28-\*_.
	
For the sensor kernel module to be enabled, include the following line in the `/boot/config.txt` file
```
	dtoverlay=w1-gpio
```
Edit the file and include the above line, then reboot, and the device should be visible

## Electric Demand - RAVEn
The **Rainforest RAVEn** device connect to the USB port on the rpi and can be used 
to read a smart electric meter in the home.  Currently, this works with San Diego Gas and Electric Meters to the best of the authors knowledge.

## Basic Switch
This can be any digital signal on a pin that is either high or low.  The tool reads the digital pin using the **wiringPi** package numbering scheme.  You will need to add the pin number to your configuration file.

## Usage
```
    $ pi2mqtt [-v] [-c FILE]
    -v - verbose mode.
    -c - configuration file
```

## Installation
To build and install the tools you will need to install the autotools suite.  For ubuntu:
```
    $ sudo apt-get update
    $ sudo apt-get install autotools-dev
```

**pi2mqtt** depends on the following libraries

* [wiringPi](http://wiringpi.com) (http://wiringpi.com)
* [paho-mqtt3](http://www.eclipse.org/paho/clients/c) (http://www.eclipse.org/paho/clients/c)
* [confuse](https://github.com/martinh/libconfuse) (https://github.com/martinh/libconfuse)


Using GIT:
```
    $ git clone https://github.com/ngongca/pi2mqtt.git
    $ autoreconf -i
    $ ./configure
    $ make
    $ sudo make install
    $ ./init_pi2mqtt
```
## Configuration
`./init_pi2mqtt` generates a template configuration file, ___template.conf___ , that will need to be edited for your configuration.  It uses the **confuse** libary syntax. Below is the syntax for the various types of sensors.
### Manditory fields
client ID for the mqtt sensor controller (usually the microcontroller id such as the PI  THIS MUST BE A UNIQUE identifier)
```
clientid = <id>
```
mqtt broker address
```
mqttbrokeraddress = "<address>"
```
mqtt broker user id if the broker requires a user authentication
```
mqttbrokeruid = "<user id>"
```
mqtt broker user id password
```
mqttbrokerpwd = "<password>"
```
topic for the sensor controller's management port. This will be monitored to provide management messages back to the controller.
```
mqttsubtopic = "<topic>"

Example
mqttsubtopci = "home/+/manage"
```
### DS18B20 config syntax
<> indicates user input.  Leading spaces are required
```
ds18b20 <identifier> {
 address = "<fully qualified directory to device>"
 mqttpubtopic = "<topic to publish>"
 sampletime = <integer in seconds>
 isfahrenheit = <1 if is, 0 if Celsius desired>
}

Example:

ds18b20 ds18b20_0 {
 address = "/sys/bus/w1/devices/28-0516a49946ff"
 mqttpubtopic = "temp"
 location = "server"
 sampletime = 4
 isfahrenheit = 1
}
```
### Switch config syntax
<> indicates user input.  Leading spaces are required
```
doorswitch <identifier> {
 pin = <wiringPi pin number>
 mqttpubtopic = "<topic to publish>"
 location = "<string location of sensor (no spaces)>"
 sampletime = <time in seconds>
}

Example:

doorswitch door1 {
 pin = 1
 mqttpubtopic = "door"
 location = "garageleft"
 sampletime = 10
}
```
### RAVEn config syntax 
<> indicates user input.  Leading spaces are required
```
RAVEn <identifier> {
 address = "<location of the serial port>"
 location = "<text>"
 mqttpubtopic = "<topic to publish>"
}

Example:

RAVEn raven0 {
 address = "/dev/ttyUSB0"
 location = "garage"
 mqttpubtopic = "demand"
}

```






