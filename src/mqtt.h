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
        int killed; ///< flag to kill loop
    } my_context_t;

    typedef struct {
        char* mqttpasswd;  ///< Password for MQTT broker account
        char* mqttuid;  ///< MQTT broker user id
        char* mqtthostaddr; ///< MQTT broker host address
        char* mqttclientid; ///< Unique client id to connect to broker.
    } mqtt_broker_t;

    typedef struct {
        char payload[MQTT_MAXPAYLOAD];  ///< payload for publishing
        char topic[MQTT_MAXTOPIC]; ///< mqtt publishing topic
    } mqtt_data_t;

    extern void delivered(void* context, MQTTClient_deliveryToken token);
    extern int msgarrvd(void* context, char* topicName, int topicLen, MQTTClient_message* message);
    extern void connlost(void* context, char* cause);
    extern void MQTT_sub(MQTTClient client, const char* topic);
    extern int mqttSend_Data(MQTTClient client, mqtt_data_t* message);
    extern int mqttSendData(MQTTClient, char *, char *);
    extern int MQTT_init(void* context, MQTTClient* client, mqtt_broker_t* broker);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_H */

