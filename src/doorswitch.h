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

/* 
 * File:   doorswitch.h
 * Author: Nick Ong <onichola@gmail.com>
 *
 * Created on May 23, 2017, 12:46 PM
 */

#ifndef DOORSWITCH_H
#define DOORSWITCH_H

#ifndef DOORSWITCH_SUCCESS
#define DOORSWITCH_SUCCESS 0
#endif

#ifndef DOORSWITCH_FAILURE
#define DOORSWITCH_FAILURE -1
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct {
        int pin; //file handle for port
        char id[64]; // id of device
        char topic[128]; // topic used for publishing
    } doorswitch_port_t;

    typedef struct {
        int is_open; // 1 indicated door open, 0 otherwise
        time_t timestamp; // timestamp of data
    } doorswitch_data_t;

    /**
     * Return a new port
     * @param pin - pin number
     * @param id - unique string identifier
     * @param topic - topic which this port will publish to mqtt
     * @return a configured port
     */
    extern doorswitch_port_t doorswitch_createPort(int, const char *, const char *);

    /**
     * Initialize the doorswitch - In this case, just set up the wiringPi.
     * @return DOORSWITCH_SUCCESS if initialization is good.
     */
    extern int doorswitch_init();

    /**
     * Read the door switch port and return the data
     * @param port
     * @param data
     * @return DOORSWITCH_SUCCESS if read was successful.
     */
    extern int read_doorswitch_dat(doorswitch_port_t, doorswitch_data_t *);

#ifdef __cplusplus
}
#endif

#endif /* DOORSWITCH_H */

