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

/* 
 * File:   tempSensor.h
 * Author: Nick Ong <onichola@gmail.com>
 *
 * Created on July 23, 2018, 10:19 PM
 */

#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

#ifndef TEMPSENSOR_SUCCESS
#define TEMPSENSOR_SUCCESS 0  ///< success indicator
#endif

#ifndef TEMPSENSOR_FAILURE
#define TEMPSENSOR_FAILURE -1  ///< failure indicator
#endif

#ifndef ADC_BASE
#define ADC_BASE 400 ///< base for ADC address
#endif

#ifndef ADC_I2C_ADDR
#define ADC_I2C_ADDR 72 ///< i2c address for ADC
#endif

#ifndef ADC_LSB
#define ADC_LSB 125e-06  ///< LSB for ADC calculation
#endif

#ifdef __cplusplus
extern "C" {
#endif

    #include "mqtt.h"

    typedef struct {
        int pin; ///< pin number of a2d this port is attached [0-4]]
	double A; ///< Parameter for Steinhart Hart equation
	double B; ///< Parameter for Steinhart Hart equation
	double C; ///< Parameter for Steinhart Hart equation
	double Rb; ///< BIAS resistor value
        char id[64]; ///< id of device
        char location[64]; ///< location of this tempsensor
        char topic[64]; ///< topic suffix used for publishing
        int sampletime; ///< sample time of this switch in seconds.
    } tempsensor_port_t;


    /**
     * \brief Create a new door switch port
     * @param pin - a2d pin number [0-4]
     * @param A; ///< Parameter for Steinhart Hart equation
     * @param B; ///< Parameter for Steinhart Hart equation
     * @param C; ///< Parameter for Steinhart Hart equation
     * @param Rb; ///< BIAS resistor value
     * @param id - unique string identifier
     * @param topic - suffix topic which this port will publish to mqtt
     * @param location - location for this port
     * @param sampletime - sample time in seconds
     * @return a configured port
     */
    extern tempsensor_port_t tempsensor_createPort(const int pin,  
	    const double A, const double B, const double C, const double Rb, const char* id,
            const char* topic, const char* location, int sampletime);

    /**
     * \brief Initialize the tempsensor - In this case, just set up the wiringPi.
     * @param port - port to init
     * @return TEMPSENSOR_SUCCESS if initialization is good.
     */
    extern int tempsensor_init();

    /**
     * \brief Process door switch port data.
     * 
     * Read state of the door switch.  If it changed, notify calling function and
     * place data into the message.  This will also change the state of the port
     * to the current state.
     * 
     * @param port door switch to read
     * @param message MQTT formated topic and data
     * @return TEMPSENSOR_SUCCESS if the state changed and the swtichy
     */
    extern int ProcessTempsensorData(tempsensor_port_t* port, mqtt_data_t* message);

#ifdef __cplusplus
}
#endif

#endif /* TEMPSENSOR_H */

