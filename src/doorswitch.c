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
#include <unistd.h>

#include "debug.h"
#include "doorswitch.h"

/**
 * Return a new port
 * @param pin - pin number
 * @param id - unique string identifier
 * @param topic - topic which this port will publish to mqtt
 * @return a configured port
 */
doorswitch_port_t
doorswitch_createPort(int pin, const char *id, const char *topic) {
    doorswitch_port_t port;
    port.pin = pin;
    strncpy(port.id, id, sizeof (port.id));
    strncpy(port.topic, topic, sizeof (port.topic));
    return (port);
}

/**
 * Initialize the door switch - In this case, just set up the wiringPi.
 * @return DOORSWITCH_SUCCESS if initialization is good.
 */
int
doorswitch_init() {
    int iErr = 0;
    int rc = DOORSWITCH_SUCCESS;
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

/**
 * Read the doorswitch port and return the data
 * @param port
 * @param data
 * @return DOORSWITCH_SUCCESS if read was successful.
 */
int
read_doorswitch_dat(doorswitch_port_t port, doorswitch_data_t *data) {

    data->timestamp = time(NULL);
    pinMode(port.pin, INPUT);
    if (digitalRead(port.pin) == 1) {
        data->is_open = 1;
    } else {
        data->is_open = 0;
    }
    return DOORSWITCH_SUCCESS;
}

