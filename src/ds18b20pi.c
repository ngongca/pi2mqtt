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

DS18B20PI_port_t 
DS18B20PI_createPort(const char *path, const char *id, const char *topic, int isFahrenheit)
{
    DS18B20PI_port_t port;
    strncpy(port.id, id, sizeof (port.id));
    strncpy(port.path, path, sizeof (port.path));
    strncpy(port.topic, topic, sizeof (port.topic));
    port.fahrenheitscale = isFahrenheit;
    return (port);
}

/**
 * Open the port for the location and store in the port variable
 * @param port - DS18B20PI_port that will be initialized and opened
 * @param location - device directory where the sensor file will be found, ex. /sys/bus/w1/devices
 * @return DS18B20PI_SUCCESS or _FAILURE
 */
int 
DS18B20PI_openPort(DS18B20PI_port_t *port) 
{
    char dbgBuf[512];
    char fullPath[512];
    int rc;

    rc = DS18B20PI_FAILURE;

    snprintf(fullPath, sizeof (fullPath), "%s/w1_slave", port->path);
    snprintf(dbgBuf, sizeof (dbgBuf), "Opening port [%s]", fullPath);
    WriteDBGLog(dbgBuf);
    port->FH = fopen(fullPath, "r");
    if (port->FH == NULL) {
        snprintf(dbgBuf, sizeof (dbgBuf), "open_port: Unable to open %s. 0x%0x - %s\n", port->path, errno, strerror(errno));
        WriteDBGLog(dbgBuf);
        perror(dbgBuf);
    } else {
        WriteDBGLog("ds18b20 Port is open");
        rc = DS18B20PI_SUCCESS;
    }
    return (rc);
} //OpenPort

/**
 * 
 * @param port
 */
void 
DS18B20PI_closePort(DS18B20PI_port_t port) 
{
    WriteDBGLog("Closing DS18B20 Port");
    fclose(port.FH);
} // ClosePort()

/**
 * Get temperature from the DS18B20 connected to the pi
 * @param port - DS18B20PI_port that is the port to read
 * @param data - DS18B20PI_data that will contain information read, temperature in F or C
 * @return - DS18B20PI_SUCCESS if read was successful, DS18B20PI_FAILURE if not.
 */
int 
DS18B20PI_getSensorTemp(DS18B20PI_port_t port, DS18B20PI_data_t *data_ptr) 
{
    char *value;
    int retval;
    char readBuf[512];
    char dbgBuf[512];
    int i;

    retval = DS18B20PI_FAILURE;
    memset(readBuf, 0, sizeof ( readBuf));
    if (fgets(readBuf, sizeof (readBuf), port.FH) > 0) {
        if (strstr(readBuf, "YES") != NULL) {
            if (fgets(readBuf, sizeof (readBuf), port.FH) > 0) {
                //Extract temp data an if Fahrenheit, convert
                if (strtok(readBuf, "\nt=") != NULL) {
                    value = strtok(NULL, "\nt=");
                    if (value != NULL && sscanf(value, "%d", &i) != 0) {
                        if (port.fahrenheitscale == 1) {
                            data_ptr->temperature = i / 1000.0 * 9.0 / 5.0 + 32.0;
                        } else {
                            data_ptr->temperature = i / 1000.0;
                        }
                        data_ptr->timestamp = time(NULL);
                        retval = DS18B20PI_SUCCESS;
                        snprintf(dbgBuf, sizeof (dbgBuf), "Extracted temperature is %.2f at timestamp %lu", data_ptr->temperature, data_ptr->timestamp);
                        WriteDBGLog(dbgBuf);
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
    return retval;
}
