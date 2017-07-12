/*
 * The MIT License
 *
 * Copyright 2017 Nick Ong onichola@gmail.com
 * ds18b20pi.h
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

/* 
 * @file:   ds18b20pi.h
 * @author: Nick Ong
 *
 * Created on April 3, 2017, 3:37 PM
 */

#ifndef DS18B20PI_H
#define DS18B20PI_H


#ifndef DS18B20PI_SUCCESS
#define DS18B20PI_SUCCESS 0
#endif

#ifndef DS18B20PI_FAILURE
#define DS18B20PI_FAILURE -1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "mqtt.h"

    typedef struct {
        char path[128]; // fully qualified path to device
        char id[64]; // id of device
        char topic[64]; // final topic to publish
        char location[64]; // location data
        int sampletime; // time in seconds to sample this sensor
        int fahrenheitscale; // 1 if Fahrenheit, 0 if Celsius 
    } DS18B20PI_port_t;

    /**
     * Create a new port
     * @param path - direct path to device including device number
     * @param id - id used to uniquely identify sensor
     * @param topic - final topic to post.  Typically "temp"
     * @param sampletime - in milliseconds
     * @param location - used for topic
     * @param isFahrenheit - determines scale of data
     * @return 
     */
    extern DS18B20PI_port_t DS18B20PI_createPort(const char *path, const char *id,
            const char *topic, const int sampletime, const char *location,
            const int isFahrenheit);
    /**
     * Initial the DS18B20 port
     * @return This is a NULL function.  Just returns DS18B20PI_SUCCESS
     */
    extern int DS18B20PI_init();

    /**
     * Process data from this ds18b20 sensor and return the data
     * @param sensorPort to process
     * @param message will contain the topic and payload for this sensor read
     * @return DS18B20PI_SUCCESS if successful, DS18B20PI_FAILURE otherwise
     */
    extern int DS18B20PI_process_data(DS18B20PI_port_t port, mqtt_data_t *message);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20PI_H */

