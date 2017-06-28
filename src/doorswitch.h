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
#define DOORSWITCH_SUCCESS 0  ///< success indicator
#endif

#ifndef DOORSWITCH_FAILURE
#define DOORSWITCH_FAILURE -1  ///< failure indicator
#endif



#ifdef __cplusplus
extern "C" {
#endif

#include "mqtt.h"

    typedef struct {
        int pin; ///< id of Wiring PI digital pin this port is attached
        char id[64]; ///< id of device
        char location[64]; ///< location of this doorswitch
        int state; ///< state of the doorswitch
        char topic[64]; ///< topic suffix used for publishing
    } doorswitch_port_t;


    /**
     * \brief Create a new door switch port
     * @param pin - pin number
     * @param id - unique string identifier
     * @param topic - suffix topic which this port will publish to mqtt
     * @param location - location for this port
     * @return a configured port
     */
    extern doorswitch_port_t doorswitch_createPort(int pin, const char* id,
            const char* topic, const char* location);

    /**
     * Initialize the doorswitch - In this case, just set up the wiringPi.
     * @return DOORSWITCH_SUCCESS if initialization is good.
     */
    extern int doorswitch_init();

    /**
     * \brief Process door switch port data.
     * 
     * Read state of the door switch.  If it changed, notify calling function and
     * place data into the message.  This will also change the state of the port
     * to the current state.
     * 
     * @param port door switch to read
     * @param message MQTT formated topic and data
     * @return DOORSWITCH_SUCCESS if the state changed and the swtichy
     */
    extern int ProcessDoorswitchData(doorswitch_port_t* port, mqtt_data_t* message);

#ifdef __cplusplus
}
#endif

#endif /* DOORSWITCH_H */

