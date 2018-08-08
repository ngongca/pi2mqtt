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

#include <MQTTAsync.h>

#define MAXPORTS 32
#define MQTT_MAXPAYLOAD 512
#define MQTT_MAXTOPIC 512

#ifndef MQTT_SUCCESS
#define MQTT_SUCCESS 0
#endif

#ifndef MQTT_FAILURE
#define MQTT_FAILURE -1
#endif

#ifndef MQTT_DUMP_FILE
#define MQTT_DUMP_FILE "./dump"
#endif

#ifdef __cplusplus
extern "C" {
#endif
    typedef struct {
        char* mqttpasswd;  ///< Password for MQTT broker account
        char* mqttuid;  ///< MQTT broker user id
        char* mqtthostaddr; ///< MQTT broker host address
        char* mqttclientid; ///< Unique client id to connect to broker.
	char* mqtthome; ///< home section of topic
	char* mqttmanagementtopic; ///< subscription topic for management
    } mqtt_broker_t;

    typedef struct {
        int killed; ///< flag to kill loop
	int reboot; ///< flag to reboot system program
	int connected; ///<flag to indicate current client is connected.
        MQTTAsync* client; ///< the current client.
        mqtt_broker_t* broker; ///< the current broker information.
	char *configFile; ///< configuration file use at boot
    } my_context_t;
    
    
    typedef struct {
        char payload[MQTT_MAXPAYLOAD];  ///< payload for publishing
        char topic[MQTT_MAXTOPIC]; ///< mqtt publishing topic
    } mqtt_data_t;

    extern void mqttSub(MQTTAsync client, const char* topic);
    extern int mqttPublish(void* context, mqtt_data_t* message);
    extern int MQTT_init(void* context);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_H */

