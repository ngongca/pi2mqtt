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

#include <stdio.h>
#include <MQTTClient.h>
#include <string.h>
#include "mqtt.h"

#define QOS          1
#define TIMEOUT      10000L

int
MQTT_init(void *context, MQTTClient *mqttClient, mqtt_broker_t *broker) {
    char buf[512];
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    rc = MQTT_SUCCESS;
    if (MQTTClient_create(mqttClient, broker->mqtthostaddr,
            broker->mqttclientid, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS) {
        snprintf(buf, sizeof (buf), "Could not create MQTT Client at %s uid %s password: %s", broker->mqtthostaddr, broker->mqttuid, broker->mqttpasswd);
        WriteDBGLog(buf);
        rc = MQTT_FAILURE;
    } else {
        MQTTClient_setCallbacks(*mqttClient, context, connlost, msgarrvd, delivered);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        conn_opts.password = broker->mqttpasswd;
        conn_opts.username = broker->mqttuid;
        if ((rc = MQTTClient_connect(*mqttClient, &conn_opts)) != MQTTCLIENT_SUCCESS) {
            snprintf(buf, sizeof (buf), "Failed to connect user: %s, password: %s, return code %d", conn_opts.username, conn_opts.password, rc);
            WriteDBGLog(buf);
            rc = MQTT_FAILURE;
        }
    }
    return rc;
}

void
MQTT_sub(MQTTClient client, const char *topic) {
    char buf[512];
    snprintf(buf, sizeof (buf), "Subscribing to topic %s using QoS%d", topic, QOS);
    WriteDBGLog(buf);
    MQTTClient_subscribe(client, topic, QOS);
}


void
delivered(void *context, MQTTClient_deliveryToken token) {
    char buf[128];
    snprintf(buf, sizeof (buf), "Message with token value %d delivery confirmed", token);
    WriteDBGLog(buf);
}

int
msgarrvd(void *context, char* topicName, int topicLen, MQTTClient_message *message) {
    char buf[128];
    char msg[128];
    my_context_t *c = (my_context_t *) context;
    int i;
    char* payloadptr;
    snprintf(buf, sizeof (buf), "Message arrived\n\ttopic: %s", topicName);
    WriteDBGLog(buf);
    payloadptr = message->payload;
    for (i = 0; i < message->payloadlen && i<sizeof (msg); i++) {
        msg[i] = *payloadptr++;
    }
    msg[i] = '\0';

    snprintf(buf, sizeof (buf), "payload %s", msg);
    WriteDBGLog(buf);
    if (strstr(msg, "kill") != NULL) {
        c->killed = 1;
    }
    WriteDBGLog("here");
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void
connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

/**
 * 
 * @param mqttClient
 * @param payload
 * @param topic
 * @return 
 */
int
mqttSendData(MQTTClient mqttClient, char *payload, char *topic) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    char dbgBuf[256];
    int rc;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    snprintf(dbgBuf, sizeof (dbgBuf), "publishing - %s", payload);
    WriteDBGLog(dbgBuf);
    MQTTClient_publishMessage(mqttClient, topic, &pubmsg, &token);
    snprintf(dbgBuf, sizeof (dbgBuf), "Waiting for up to %d seconds for publication of %s on topic %s",
            (int) (TIMEOUT / 1000), payload, topic);
    WriteDBGLog(dbgBuf);
    rc = MQTTClient_waitForCompletion(mqttClient, token, TIMEOUT);
    snprintf(dbgBuf, sizeof (dbgBuf), "Message with delivery token %d delivered", token);
    WriteDBGLog(dbgBuf);
}

/**
 * 
 * @param mqttClient
 * @param payload
 * @param topic
 * @return 
 */
int
mqttSend_Data(MQTTClient client, mqtt_data_t *message) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    char dbgBuf[256];
    int rc;
    pubmsg.payload = message->payload;
    pubmsg.payloadlen = strlen(message->payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    snprintf(dbgBuf, sizeof (dbgBuf), "publishing - %s", message->payload);
    WriteDBGLog(dbgBuf);
    MQTTClient_publishMessage(client, message->topic, &pubmsg, &token);
    snprintf(dbgBuf, sizeof (dbgBuf), "Waiting for up to %d seconds for publication of %s on topic %s",
            (int) (TIMEOUT / 1000), message->payload, message->topic);
    WriteDBGLog(dbgBuf);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    snprintf(dbgBuf, sizeof (dbgBuf), "Message with delivery token %d delivered", token);
    WriteDBGLog(dbgBuf);
}

