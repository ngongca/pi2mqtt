/*
 * The MIT License
 *
 * Copyright 2017 Nick Ong  onichola@gmail.com
 * ds18b20pi.c
 * 
 * An implementation to read temperature
 *
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ds18b20pi.h"
#include "debug.h"
#include "mqtt.h"

int
DS18B20PI_init() {
    return DS18B20PI_SUCCESS;
}

DS18B20PI_port_t
DS18B20PI_createPort(const char *path, const char *id, const char *topic, const int sampletime, const char *location, const int isFahrenheit) {
    DS18B20PI_port_t port;
    strncpy(port.id, id, sizeof (port.id));
    strncpy(port.path, path, sizeof (port.path));
    snprintf(port.topic, sizeof (port.topic), "home/%s/%s/%s", id, location, topic);
    strncpy(port.location, location, sizeof (port.location));
    port.sampletime = sampletime;
    port.fahrenheitscale = isFahrenheit;
    return (port);
}

int
DS18B20PI_process_data(DS18B20PI_port_t port, mqtt_data_t *message) {
    int rc;
    char *value;
    char dbgBuf[1024];
    char readBuf[512];
    char fullPath[512];
    float temp;
    int i;
    FILE* fh;
    
    rc = DS18B20PI_FAILURE;
    snprintf(fullPath, sizeof (fullPath), "%s/w1_slave", port.path);
    snprintf(dbgBuf, sizeof (dbgBuf), "Opening port [%s]", fullPath);
    WriteDBGLog(dbgBuf);
    // to read the ds18b20, you need to re-open the file
    fh = fopen(fullPath, "r");
    if (fh == NULL) {
        snprintf(dbgBuf, sizeof (dbgBuf), "open_port: Unable to open %s. 0x%0x - %s\n", fullPath, errno, strerror(errno));
        WriteDBGLog(dbgBuf);
        perror(dbgBuf);
    } else {
        memset(readBuf, 0, sizeof ( readBuf));
        if (fgets(readBuf, sizeof (readBuf), fh) > 0) {
            if (strstr(readBuf, "YES") != NULL) {
                if (fgets(readBuf, sizeof (readBuf), fh) > 0) {
                    //Extract temp data an if Fahrenheit, convert
                    if (strtok(readBuf, "\nt=") != NULL) {
                        value = strtok(NULL, "\nt=");
                        if (value != NULL && sscanf(value, "%d", &i) != 0) {
                            if (port.fahrenheitscale == 1) {
                                temp = i / 1000.0 * 9.0 / 5.0 + 32.0;
                            } else {
                                temp = i / 1000.0;
                            }
                            snprintf(message->payload, sizeof (message->payload), "{\"timestamp\":%ld,\"value\":%.3f}", time(NULL), temp);
                            strncpy(message->topic, port.topic, sizeof (message->topic));
                            snprintf(dbgBuf, sizeof (dbgBuf), "Set up payload to %s", message->payload);
                            WriteDBGLog(dbgBuf);
                            rc = DS18B20PI_SUCCESS;
                        } else {
                            WriteDBGLog("No temp scanned");
                        }
                    } else {
                        WriteDBGLog("no number scanned");
                    }
                }
            } else {
                snprintf(dbgBuf, sizeof (dbgBuf), "Bad temp reading CRC Check failed on %s", readBuf);
                WriteDBGLog(dbgBuf);
            }
        } else {
            WriteDBGLog("No data Read");
        }
        WriteDBGLog("Closing DS18B20 Port");
        fclose(fh);
        return (rc);
    }
}

