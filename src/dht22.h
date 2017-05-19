/*
 * The MIT License
 *
 * Copyright 2017 Nick Ong.
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
 * File:   dht22.h
 * Author: Nick Ong  onichola@gmail.com
 *
 * Created on May 14, 2017, 8:15 PM
 */

#ifndef DHT22_H
#define DHT22_H

#ifndef DHT22_SUCCESS
#define DHT22_SUCCESS 0
#endif

#ifndef DHT22_FAILURE
#define DHT22_FAILURE -1
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        int pin; //file handle for port
        char id[64]; // id of device
        char topic[128]; // root of topic, will concat Temp and Humidity
        int fahrenheitscale; // use fahrenheit
    } dht22_port_t;

    typedef struct {
        float temperature; // temperature
        float humidity; // humidity
        time_t timestamp; // timestamp of data
    } dht22_data_t;

    /**
     * Return a new port
     * @param pin - pin number
     * @param id - unique string identifier
     * @param topic - topic which this port will publish to mqtt
     * @param isFahrenheit - set to 1 if F, otherwise C
     * @return a configured port
     */
    extern dht22_port_t dht22_createPort(int, const char *, const char *, int);

    /**
     * Initialize the DHT22 - In this case, just set up the wiringPi.
     * @return DHT22_SUCCESS if initialization is good.
     */
    extern int dht22_init();

    /**
     * Read the DHT22 port and return the data
     * @param port
     * @param data
     * @return DHT22_SUCCESS if read was successful.
     */
    extern int read_dht22_dat(dht22_port_t, dht22_data_t *);

#ifdef __cplusplus
}
#endif

#endif /* DHT22_H */

