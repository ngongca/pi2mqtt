/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ds18b20pi.h
 * Author: nick
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
        FILE *FH; //file handle for port
        char path[128]; // fully qualified path to device
        char id[64]; // id of device
        char topic[64]; // final topic to publish
        char location[64]; // location data
        int sampletime; // time in seconds to sample this sensor
        int fahrenheitscale; // 1 if Fahrenheit, 0 if Celsius 
    } DS18B20PI_port_t;

    extern DS18B20PI_port_t DS18B20PI_createPort(const char *, const char *,
            const char *, const int, const char *, const int);
    extern int DS18B20PI_openPort(DS18B20PI_port_t *);
    extern void DS18B20PI_closePort(DS18B20PI_port_t);
    extern int ProcessDS18B20PIData(DS18B20PI_port_t, mqtt_data_t *);


#ifdef __cplusplus
}
#endif

#endif /* DS18B20PI_H */

