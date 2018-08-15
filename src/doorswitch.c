/*
 * The MIT License
 *
 * Copyright 2017 Nick Ong <onichola@gmail.com>.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"
#include "doorswitch.h"
#include "mqtt.h"

/**
 * Return a new port
 * @param pin - pin number
 * @param id - unique string identifier
 * @param topic - topic which this port will publish to mqtt
 * @return a configured port
 */
doorswitch_port_t
doorswitch_createPort(int pin, const char *id, const char *topic, 
        const char* location, int sampletime, int sampleContinous) {
    char dbgBuf[256];
    doorswitch_port_t port;
    snprintf(dbgBuf, sizeof (dbgBuf), "Creating door switch %s pin %d at %s", id, pin, location);
    WriteDBGLog(dbgBuf);    
    port.pin = pin;
    port.state = -1;
    port.sampletime = sampletime;
    port.sampleContinuous = sampleContinous;
    strncpy(port.id, id, sizeof (port.id));
    strncpy(port.topic, topic, sizeof (port.topic));
    strncpy(port.location, location, sizeof (port.location));
    return (port);
}

int
doorswitch_init() {
    int iErr = 0;
    int rc = DOORSWITCH_SUCCESS;
    WriteDBGLog("initializing Door Switches");
    iErr = wiringPiSetup();
    if (iErr == -1) {
        WriteDBGLog("doorswitch : Error Failed to init WiringPi");
        rc = DOORSWITCH_FAILURE;
    }
    if (setuid(getuid()) < 0) {
        WriteDBGLog("doorswitch : Error Dropping privileges failed\n");
        rc = DOORSWITCH_FAILURE;
    }
    return (rc);
}

int
ProcessDoorswitchData(doorswitch_port_t* port, mqtt_data_t* message) {
    char dbgBuf[256];
    
    int rc = DOORSWITCH_FAILURE;
    pinMode(port->pin, INPUT);
    int data = digitalRead(port->pin);
    if (data != port->state) {
        snprintf(dbgBuf, sizeof (dbgBuf), "Door %s changed to state %d", port->id, data);
        WriteDBGLog(dbgBuf);
        port->state = data;
        snprintf(message->payload, sizeof (message->payload), 
                "{\"timestamp\":%ld,\"value\":\"%s\"}",
                time(NULL), data == 1 ? "opened" : "closed");
        snprintf(message->topic, sizeof (message->topic), "%s/%s/%s",
                port->id, port->location, port->topic);
        rc = DOORSWITCH_SUCCESS;
    } else if ( port->sampleContinuous == 1 ) {
	snprintf(dbgBuf, sizeof (dbgBuf), "Door %s state %d", port->id, data);
        WriteDBGLog(dbgBuf);
        port->state = data;
        snprintf(message->payload, sizeof (message->payload), 
                "{\"timestamp\":%ld,\"value\":\"%s\"}",
                time(NULL), data == 1 ? "opened" : "closed");
        snprintf(message->topic, sizeof (message->topic), "%s/%s/%s",
                port->id, port->location, port->topic);
        rc = DOORSWITCH_SUCCESS;
    }
    return rc;
}

