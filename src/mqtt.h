/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   mqtt.h
 * Author: Nick Ong <onichola@gmail.com>
 *
 * Created on June 24, 2017, 1:58 PM
 */

#ifndef MQTT_H
#define MQTT_H

#define MAXPORTS 32
#define MQTT_MAXPAYLOAD 1024
#define MQTT_MAXTOPIC 64

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* mqttpasswd;
    char* mqttuid;
    char* mqtthostaddr;
    char* mqttclientid;
} mqtt_broker_t;

typedef struct {
    char payload[MQTT_MAXPAYLOAD];
    char topic[MQTT_MAXTOPIC];
} mqtt_data_t;

#ifdef __cplusplus
}
#endif

#endif /* MQTT_H */

