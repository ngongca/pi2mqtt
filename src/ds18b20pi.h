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

    typedef struct {
        FILE *FH; //file handle for port
        char path[128]; // fully qualified path to device
        char id[64]; // id of device
        char topic[128]; // fully qualified topic
        int fahrenheitscale; // 1 if Fahrenheit, 0 if Celsius 
    } DS18B20PI_port_t;

    typedef struct {
        float temperature; // temperature
        time_t timestamp; // timestamp of data
    } DS18B20PI_data_t;

    extern DS18B20PI_port_t DS18B20PI_createPort(const char *, const char *, const char *, int);
    extern int DS18B20PI_openPort(DS18B20PI_port_t *);
    extern void DS18B20PI_closePort(DS18B20PI_port_t);
    extern int DS18B20PI_getSensorTemp(DS18B20PI_port_t, DS18B20PI_data_t *);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20PI_H */

