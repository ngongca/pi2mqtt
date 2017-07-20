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

static int connected = 0;

static void
connect_client(MQTTClient* client, mqtt_broker_t* broker) {
    char buf[128];
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.password = broker->mqttpasswd;
    conn_opts.username = broker->mqttuid;

    WriteDBGLog("Attempting to connect");
    while ((MQTTClient_connect(*client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        snprintf(buf, sizeof (buf),
                "Failed to connect user: %s, password: %s",
                conn_opts.username, conn_opts.password);
        WriteDBGLog(buf);
        WriteDBGLog("Re-attempt in 10 seconds");
        sleep(10);
        WriteDBGLog("Attempting to connect");
    }
    WriteDBGLog("Connection to Broker Successful");
}

static void
connlost(void *context, char *cause) {
    my_context_t* c = (my_context_t *) context;
    FILE *fp;
    char buf[1024];
    mqtt_data_t data;
    char *filename = "./dump";
    char *topic = "home/pi2mqtt/mgmnt";
    
    connected = 0;
    
    snprintf(buf, 64, "Broker connection lost. Cause: %s\n", cause);
    WriteDBGLog(buf);
    fp = fopen(filename, "w");
    fclose(fp);
        
    // Let the broker know when the connection was lost
    snprintf(data.payload,sizeof(data.payload), 
            "{\"timestamp\":%ld,\"connection\":\"lost\"}", time(NULL));
    snprintf(data.topic,sizeof(data.topic),"%s",topic);
    MQTT_send(*c->client, &data);
    
    connect_client(c->client, c->broker);
    connected = 1;
    if ((fp = fopen(filename, "r")) != NULL) {
        while (fgets(buf, sizeof (buf), fp) != NULL) {
            sscanf(buf, "%s\n", data.topic);
            fgets(buf, sizeof (buf), fp);
            sscanf(buf, "%s\n", data.payload);
            sprintf(buf, "publishing stored data to %s", data.topic);
            WriteDBGLog(buf);
            MQTT_send(*c->client, &data);
        }
        fclose(fp);
    }
    snprintf(data.payload,sizeof(data.payload), 
            "{\"timestamp\":%ld,\"connection\":\"reconnected\"}", time(NULL));
    snprintf(data.topic,sizeof(data.topic),"%s",topic);
    MQTT_send(*c->client, &data);
}

static void
delivered(void *context, MQTTClient_deliveryToken token) {
    char buf[128];
    snprintf(buf, sizeof (buf), "Message with token value %d delivery confirmed", token);
    WriteDBGLog(buf);
}

static int
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

static void
mqtt_save(const mqtt_data_t msg) {
    FILE *fp;
    char *filename = "./dump";

    if ((fp = fopen(filename, "a")) == NULL) {
        WriteDBGLog("error opening persistence file");
    }

    WriteDBGLog("Saving msg to file");

    fprintf(fp, "%s\n", msg.topic);
    fprintf(fp, "%s\n", msg.payload);

    fclose(fp);
}

void
MQTT_sub(MQTTClient client, const char *topic) {
    char buf[512];
    snprintf(buf, sizeof (buf), "Subscribing to topic %s using QoS%d", topic, QOS);
    WriteDBGLog(buf);
    MQTTClient_subscribe(client, topic, QOS);
}

/**
 * 
 * @param mqttClient
 * @param payload
 * @param topic
 * @return 
 */
int
MQTT_send(MQTTClient client, mqtt_data_t *message) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    char dbgBuf[256];
    int rc;

    if (connected == 1) {
        pubmsg.payload = message->payload;
        pubmsg.payloadlen = strlen(message->payload);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        snprintf(dbgBuf, sizeof (dbgBuf), "publishing - %s", message->payload);
        WriteDBGLog(dbgBuf);
        int i = MQTTClient_publishMessage(client, message->topic, &pubmsg, &token);
        snprintf(dbgBuf, sizeof (dbgBuf), "Waiting for up to %d seconds for publication of %s on topic %s with token %d",
                (int) (TIMEOUT / 1000), message->payload, message->topic, token);
        WriteDBGLog(dbgBuf);
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        snprintf(dbgBuf, sizeof (dbgBuf), "Message with delivery token %d delivered", token);
        WriteDBGLog(dbgBuf);
    } else
        mqtt_save(*message);
}

int
MQTT_init(void* context, MQTTClient *mqttClient, mqtt_broker_t *broker) {
    char buf[512];
    int rc;

    rc = MQTT_SUCCESS;
    ///@todo Implement Persistence 
    if (MQTTClient_create(mqttClient, broker->mqtthostaddr, broker->mqttclientid,
            MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS) {
        snprintf(buf, sizeof (buf), "Could not create MQTT Client at %s uid %s password: %s", broker->mqtthostaddr, broker->mqttuid, broker->mqttpasswd);
        WriteDBGLog(buf);
        connected = 0;
        rc = MQTT_FAILURE;
    } else {
        MQTTClient_setCallbacks(*mqttClient, context, connlost, msgarrvd, delivered);
        connect_client(mqttClient, broker);
        connected = 1;
        rc = MQTT_SUCCESS;
    }
    return rc;
}