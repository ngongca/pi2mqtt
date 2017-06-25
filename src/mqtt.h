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

#include <MQTTClient.h>

#define MAXPORTS 32
#define MQTT_MAXPAYLOAD 1024
#define MQTT_MAXTOPIC 64

#ifndef MQTT_SUCCESS
#define MQTT_SUCCESS 0
#endif

#ifndef MQTT_FAILURE
#define MQTT_FAILURE -1
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        int killed; // flag to kill loop
    } my_context_t;

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

    extern void delivered(void *, MQTTClient_deliveryToken);
    extern int msgarrvd(void *, char *, int, MQTTClient_message *);
    extern void connlost(void *, char *);
    extern void MQTT_sub(MQTTClient, char *);
    extern int mqttSend_Data(MQTTClient, mqtt_data_t *);
    extern int mqttSendData(MQTTClient, char *, char *);
    extern int MQTT_init(void *, MQTTClient *, mqtt_broker_t *);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_H */

