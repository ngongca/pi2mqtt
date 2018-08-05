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
 * @param id - unique string identifier
 * @param topic - topic which this port will publish to mqtt
 * @return a configured port
 */
tempsensor_port_t
tempsensor_createPort(
	const int pin,
	const double A,
	const double B,
	const double C,
	const double Rb,
	const char *id,
	const char *topic,
	const char* location,
	int sampletime) {
    char dbgBuf[256];
    tempsensor_port_t port;
    snprintf(dbgBuf, sizeof (dbgBuf), "Creating door switch %s pin %d at %s", id, pin, location);
    WriteDBGLog(dbgBuf);
    port.pin = pin;
    port.sampletime = sampletime;
    port.A = A;
    port.B = B;
    port.C = C;
    port.Rb = Rb;
    strncpy(port.id, id, sizeof (port.id));
    strncpy(port.topic, topic, sizeof (port.topic));
    strncpy(port.location, location, sizeof (port.location));
    return (port);
}

int
tempsensor_init() {
    int iErr = 0;
    int rc = TEMPSENSOR_SUCCESS;
    WriteDBGLog("initializing ADC for tempsensor");
    iErr = wiringPiSetup();
    if (iErr == -1) {
	WriteDBGLog("tempsensor : Error Failed to init WiringPi");
	rc = TEMPSENSOR_FAILURE;
    }
    if (setuid(getuid()) < 0) {
	WriteDBGLog("tempsensor : Error Dropping privileges failed\n");
	rc = TEMPSENSOR_FAILURE;
    }
    if (ads1115Setup(ADC_BASE, ADC_I2C_ADDR) == FALSE) {
	WriteDBGLog("tempsensor : Error initializing ads1115 ADC\n");
	rc = TEMPSENSOR_FAILURE;
    }
    return (rc);
}

double
NtcR2T(double r, double A, double B, double C) {
    char dbgBuf[512];
    snprintf(dbgBuf, sizeof (dbgBuf), "tempsensor: Calculating temp A %e, B %e, C %e, R %.4f", A, B, C, r);
    WriteDBGLog(dbgBuf);
    double lr = log(r);
    double B1 = B * lr;
    double plr = pow(lr, 3.0);
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
    int p = ADC_BASE + port->pin;
    snprintf(dbgBuf, sizeof (dbgBuf), "tempsensor: Reading ADC port %d", p);
    WriteDBGLog(dbgBuf);
    int data = analogRead(p);
    snprintf(dbgBuf, sizeof (dbgBuf), "tempsensor: Reading ADC value 0x%04x or %d", data, data);
    WriteDBGLog(dbgBuf);

    // Get resistance value assumes 10kohm to ground for v
    double v = data * ADC_LSB;
    double R = 33000.0 / v - 10000.0 - port->Rb;
    
    double t = NtcR2T(R, port->A, port->B, port->C);
    double tF = t * 9.0 / 5.0 + 32.0;
    snprintf(dbgBuf, sizeof (dbgBuf),
	    "tempsensor: data = %d, ADC voltage = %.4f, Resistance = %.4f, temp = %.2fC or %.2fF", data, v, R, t, tF);
    WriteDBGLog(dbgBuf);
    snprintf(message->payload, sizeof (message->payload),
	    "{\"timestamp\":%ld,\"value\":\"%.2f\"}",
	    time(NULL), t);
    snprintf(message->topic, sizeof (message->topic), "%s/%s/%s",
	    port->id, port->location, port->topic);
    rc = TEMPSENSOR_SUCCESS;
    return rc;
}


