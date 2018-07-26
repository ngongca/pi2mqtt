/*
 * The MIT License
 *
 * Copyright 2018 Nick Ong <onichola@gmail.com>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <wiringPi.h>
#include <ads1115.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "debug.h"
#include "tempsensor.h"
#include "mqtt.h"

/**
 * Return a new port
 * @param base - base pin number
 * @param pin - pin number [0-4]
 * @param addr - i2c address used
 * @param id - unique string identifier
 * @param topic - topic which this port will publish to mqtt
 * @return a configured port
 */
tempsensor_port_t
tempsensor_createPort(const int base, const int pin, const int addr, const char *id, const char *topic,
	const char* location, int sampletime) {
    char dbgBuf[256];
    tempsensor_port_t port;
    snprintf(dbgBuf, sizeof (dbgBuf), "Creating door switch %s pin %d at %s", id, pin, location);
    WriteDBGLog(dbgBuf);
    port.base = base;
    port.pin = pin;
    port.addr = addr;
    port.sampletime = sampletime;
    strncpy(port.id, id, sizeof (port.id));
    strncpy(port.topic, topic, sizeof (port.topic));
    strncpy(port.location, location, sizeof (port.location));
    return (port);
}

int
tempsensor_init(tempsensor_port_t port) {
    int iErr = 0;
    int rc = TEMPSENSOR_SUCCESS;
    WriteDBGLog("initializing Door Switches");
    iErr = wiringPiSetup();
    if (iErr == -1) {
	WriteDBGLog("tempsensor : Error Failed to init WiringPi");
	rc = TEMPSENSOR_FAILURE;
    }
    if (setuid(getuid()) < 0) {
	WriteDBGLog("tempsensor : Error Dropping privileges failed\n");
	rc = TEMPSENSOR_FAILURE;
    }
    if (ads1115Setup(port.base, port.addr) == FALSE) {
	WriteDBGLog("tempsensor : Error initializing ads1115 ADC\n");
	rc = TEMPSENSOR_FAILURE;
    }
    return (rc);
}

double
NtcR2T(double r) {
     char dbgBuf[512];
    double A = 1.413e-3;
    double B = 2.385e-4;
    double C = 9.588e-8;
    double lr = log(r);
    double B1 = B * lr;
    double plr = pow(lr,3.0);
    snprintf(dbgBuf, sizeof(dbgBuf), "tempsensor: LogR = %.4f LogR^3 = %.4f", lr, plr);
    WriteDBGLog(dbgBuf);
    double C1 = C * plr;
    double s = A + B1 + C1;
    double tK = 1.0 / s;
    double t = tK - 273.15;
    return t;
}

int
ProcessTempsensorData(tempsensor_port_t* port, mqtt_data_t* message) {
    char dbgBuf[512];

    int rc = TEMPSENSOR_FAILURE;
    int p = port->base + port->pin;
    snprintf(dbgBuf, sizeof(dbgBuf), "tempsensor: Reading ADC port %d", p);
    WriteDBGLog(dbgBuf);
    int data = analogRead(p);
    snprintf(dbgBuf, sizeof(dbgBuf), "tempsensor: Reading ADC value 0x%04x or %d", data, data);
    WriteDBGLog(dbgBuf);
    
    double scale = 0x7fff;
    double v = data * 125e-6; // LSB
    double R = 33000.0/v - 10000.0;
    double t = NtcR2T(R);
    double tF = t*9.0/5.0 + 32.0;
    snprintf(dbgBuf, sizeof(dbgBuf), 
	    "tempsensor: scale = %.2f, data = %d, ADC voltage = %.4f, Resistance = %.4f, temp = %.2fC or %.2fF", 
	    scale, data, v,R, t, tF);
    WriteDBGLog(dbgBuf);
    snprintf(message->payload, sizeof (message->payload),
	    "{\"timestamp\":%ld,\"value\":\"%.2f\"}",
	    time(NULL), tF);
    snprintf(message->topic, sizeof (message->topic), "home/%s/%s/%s",
	    port->id, port->location, port->topic);
    rc = TEMPSENSOR_SUCCESS;
    return rc;
}


