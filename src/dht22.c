/*
 * The MIT License
 *
 * Copyright 2017 Nick Ong.
 * dht22.c
 * 
 * A library DHT22 Temperature and Humidity Sensor
 *
 * Previous Author     : technion@lolware.net, yexiaobo@seeedstudio.com
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
#include "mqtt.h"
#include "dht22.h"

#define MAXTIMINGS 85

static int dht22_dat[5] = {0, 0, 0, 0, 0};
static int portpin;

/**
 * Return a new port
 * @param pin - pin number
 * @param id - unique string identifier
 * @param topic - topic which this port will publish to mqtt
 * @return a configured port
 */
dht22_port_t
DHT22_create(int pin, const char *id, const char *topic, int isFahrenheit) {
    dht22_port_t port;
    port.pin = pin;
    strncpy(port.id, id, sizeof (port.id));
    strncpy(port.topic, topic, sizeof (port.topic));
    port.fahrenheitscale = isFahrenheit;
    return (port);
}

static uint8_t
sizecvt(const int read) {
    /* digitalRead() and friends from wiringpi are defined as returning a value
       < 256. However, they are returned as int() types. This is a safety function */

    if (read > 255 || read < 0) {
        WriteDBGLog("dht22: Invalid data from wiringPi library\n");
        exit(EXIT_FAILURE);
    }
    return (uint8_t) read;
}

/**
 * Read the DHT22 port and return the data
 * @param port
 * @param data
 * @return DHT22_SUCCESS if read was successful.
 */
static int
read_dht22_dat(dht22_port_t port, dht22_data_t *data) {
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    char buf[256];
    
    snprintf(buf, sizeof (buf), "dht22: Reading from pin %d\n", port.pin);
    WriteDBGLog(buf);
    
    dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

    // pull pin down for 18 milliseconds
    pinMode(port.pin, OUTPUT);
    digitalWrite(port.pin, LOW);
    delay(18);

    // then pull it up for 40 microseconds
    digitalWrite(port.pin, HIGH);
    delayMicroseconds(20);

    // prepare to read the pin
    pinMode(port.pin, INPUT);

    // detect change and read data
    for (i = 0; i < MAXTIMINGS; i++) {
        counter = 0;
        while (digitalRead(port.pin) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        laststate = digitalRead(port.pin);

        if (counter == 255) {
            WriteDBGLog("dht22: counter overflow");
            break;
        }

        // ignore first 3 transitions
        if ((i >= 4) && (i % 2 == 0)) {
            // shove each bit into the storage bytes
            dht22_dat[j / 8] <<= 1;
            if (counter > 12)
                dht22_dat[j / 8] |= 1;
            j++;
        }
    }

    snprintf(buf, sizeof (buf), "dht22: data read 0x%X 0x%X 0x%x 0x%x 0x%x\n", 
            dht22_dat[0], dht22_dat[1], dht22_dat[2], dht22_dat[3], dht22_dat[4]);
    WriteDBGLog(buf);
    snprintf(buf, sizeof (buf), "dht22: Read %d bits\n", j);
    WriteDBGLog(buf);
    
    // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
    // print it out if data is good
    if ((j >= 40) && (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF))) {
        data->humidity = (float) dht22_dat[0] * 256 + (float) dht22_dat[1];
        data->humidity /= 256.0;
        data->temperature = (float) (dht22_dat[2] & 0x7F) + (float) dht22_dat[3] / 256;
        if ((dht22_dat[2] & 0x80) != 0) data->temperature *= -1;
        if (port.fahrenheitscale == 1) data->temperature = data->temperature * 9 / 5 + 32;
        return DHT22_SUCCESS;
    } else {
        return DHT22_FAILURE;
    }
}

int
DHT22_init(void* port) {
    int iErr = 0;
    int rc = DHT22_SUCCESS;
    iErr = wiringPiSetup();
    if (iErr == -1) {
        WriteDBGLog("dht22 : Error Failed to init WiringPi");
        rc = DHT22_FAILURE;
    }
    if (setuid(getuid()) < 0) {
        WriteDBGLog("dht22 : Error Dropping privileges failed\n");
        rc = DHT22_FAILURE;
    }
    return (rc);
}

int
DHT22_process_temperature(dht22_port_t dht22, mqtt_data_t* mdata) {
    dht22_data_t data;

    char dbgBuf[512];

    WriteDBGLog("Starting to PROCESS dht22 input");
    if ((read_dht22_dat(dht22, &data)) == DHT22_SUCCESS) {
        snprintf(mdata->payload, sizeof (mdata->payload), "{\"temperature\":{\"timestamp\":%ld,\"value\":%.3f}}", data.timestamp, data.temperature);
        snprintf(mdata->topic, sizeof (mdata->topic), "%s/temperature", dht22.topic);
        snprintf(dbgBuf, sizeof (dbgBuf), "Topic %s Payload %s", mdata->topic, mdata->payload);
        WriteDBGLog(dbgBuf);
    } else {
        WriteDBGLog("Failed to read dht22 sensor");
        return (DHT22_FAILURE);
    }
    return (DHT22_SUCCESS);
}

int
DHT22_process_humidity(dht22_port_t dht22, mqtt_data_t* mdata) {
    dht22_data_t data;

    char dbgBuf[512];

    WriteDBGLog("Starting to PROCESS dht22 input");
    if ((read_dht22_dat(dht22, &data)) == DHT22_SUCCESS) {
        snprintf(mdata->payload, sizeof (mdata->payload), "{\"humidity\":{\"timestamp\":%ld,\"value\":%.3f}}", data.timestamp, data.humidity);
        snprintf(mdata->topic, sizeof (mdata->topic), "%s/humidity", dht22.topic);
        snprintf(dbgBuf, sizeof (dbgBuf), "Topic %s Payload %s", mdata->topic, mdata->payload);
        WriteDBGLog(dbgBuf);
    } else {
        WriteDBGLog("Failed to read dht22 sensor");
        return (DHT22_FAILURE);
    }
    return (DHT22_SUCCESS);
}

