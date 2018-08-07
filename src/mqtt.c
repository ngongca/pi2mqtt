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
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "mqtt.h"
#include "debug.h"

#define QOS          1
#define TIMEOUT      10000L

static char* dumpFilename = MQTT_DUMP_FILE;

static void
onConnectFailure(void* context, MQTTAsync_failureData* response) {
    char buf[256];
    my_context_t *c = (my_context_t *) context;
    snprintf(buf, sizeof (buf), "Connect failed, rc %d\n", response ? response->code : 0);
    WriteDBGLog(buf);
}

static void
onConnect(void* context, MQTTAsync_successData* response) {
    my_context_t *c = (my_context_t *) context;
    c->connected = 1;
    WriteDBGLog("Successful connection");
    MQTT_sub(c, c->broker->mqttmanagementtopic);
}

void
onSubscribe(void* context, MQTTAsync_successData* response) {
    printf("Subscribe succeeded\n");
}

static void
onReconnect(void* context, char* response) {
    my_context_t *c = (my_context_t *) context;
    FILE *fp;
    char buf[1024];
    mqtt_data_t data;
    
struct timespec delay;
    delay.tv_nsec = 100000;
    delay.tv_sec = 0;

    // connected = 0 is first time through, not a reconnect.
    if (c->connected == 0) {
	c->connected = 1;
	WriteDBGLog("Successful reconnection");
	snprintf(data.payload, sizeof (data.payload),
		"{\"timestamp\":%ld,\"connection\":\"reconnected\"}", time(NULL));
	snprintf(data.topic, sizeof (data.topic), "%s", "manage");
	MQTT_send(c, &data);
    }
}

static void
onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    char buf[128];
    my_context_t *c = (my_context_t *) context;
    snprintf(buf, sizeof (buf), "Subscribe failed, rc %d\n", response ? response->code : 0);
    WriteDBGLog(buf);
    c->killed = 1;
}

static void
onConnlost(void *context, char *cause) {
    my_context_t* c = (my_context_t *) context;
    FILE *fp;
    char buf[1024];
    mqtt_data_t data;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    c->connected = 0;
    snprintf(buf, 64, "Broker connection lost. Cause: %s\n", cause);
    WriteDBGLog(buf);
    fp = fopen(dumpFilename, "w");
    fclose(fp);

    // Let the broker know when the connection was lost
    snprintf(data.payload, sizeof (data.payload),
	    "{\"timestamp\":%ld,\"connection\":\"lost\"}", time(NULL));
    snprintf(data.topic, sizeof (data.topic), "%s", "manage");
    MQTT_send(c, &data);

    /*    conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onReconnect;
	conn_opts.onFailure = onConnectFailure; 
	if ((rc = MQTTAsync_reconnect(*c->client)) != MQTTASYNC_SUCCESS) {
	    snprintf(buf, sizeof (buf), "Failed to start connect, return code %d\n", rc);
	    WriteDBGLog(buf);
	    c->killed = 1;
	} */
}

static int
onMsgArrvd(void *context, char* topicName, int topicLen, MQTTAsync_message *message) {
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
    WriteDBGLog("pi2mqtt killed via mqtt topic");
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void
onSend(void *context, MQTTAsync_successData* response) {
    my_context_t *c = (my_context_t *) context;
    char buf[128];
    snprintf(buf, sizeof (buf), "Message with token value %d delivery confirmed\n", response->token);
    WriteDBGLog(buf);
}

static void
mqtt_save(void *context, const mqtt_data_t msg) {
    my_context_t *c = (my_context_t *) context;
    FILE *fp;
    char buf[256];
    
    if ((fp = fopen(dumpFilename, "a")) == NULL) {
	WriteDBGLog("error opening persistence file");
	return;
    }
    snprintf(buf, sizeof(buf), "Saving topic %s message %s to file", msg.topic, msg.payload);
    WriteDBGLog(buf);
    fprintf(fp, "%s\n", msg.topic);
    fprintf(fp, "%s\n", msg.payload);
    fclose(fp);
}

void
MQTT_sub(void *context, const char *topic) {
    char buf[512];
    my_context_t *c = (my_context_t *) context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc;
    if (c->connected == 1) {
	snprintf(buf, sizeof (buf), "Subscribing to topic %s\nfor client %s using QoS %d",
		topic, c->broker->mqttclientid, QOS);
	WriteDBGLog(buf);
	opts.onFailure = onSubscribeFailure;
	opts.onSuccess = onSubscribe;
	opts.context = c;

	if ((rc = MQTTAsync_subscribe(*c->client, topic, QOS, &opts)) != MQTTASYNC_SUCCESS) {
	    char buf[256];
	    snprintf(buf, sizeof (buf), "Failed to start subscribe, return code %d", rc);
	    WriteDBGLog(buf);
	    exit(MQTTASYNC_FAILURE);
	}
    } else {
	WriteDBGLog("MQTT_sub not connected");
    }
}

/**
 * 
 * @param mqttClient
 * @param payload
 * @param topic
 * @return 
 */
int
MQTT_send(void *context, mqtt_data_t *message) {
    MQTTAsync_token token;
    my_context_t *c = (my_context_t *) context;
    char buf[256];

 /*   if (c->connected == 1) { */
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;
	opts.onSuccess = onSend;
	opts.context = c;
	pubmsg.payload = message->payload;
	pubmsg.payloadlen = strlen(message->payload);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	token = 0;
	
	snprintf(buf, sizeof(buf), "MQTT_send %s to %s/%s", message->payload, c->broker->mqtthome, message->topic);
	WriteDBGLog(buf);
	snprintf(buf, sizeof(buf), "%s/%s", c->broker->mqtthome, message->topic);
	if ((token = MQTTAsync_sendMessage(*c->client, buf, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
	    snprintf(buf, sizeof (buf), "Failed to start sendMessage, return code %d\n", token);
	    WriteDBGLog(buf);
	    c->killed = 1;
	    return (MQTT_FAILURE);
	}
/*
    } else {
	mqtt_save(c->broker, *message);
    }
 */
    return (MQTT_SUCCESS);
}

int
MQTT_init(void* context) {
    char buf[512];
    int rc;

    my_context_t *c = (my_context_t *) context;

    c->connected = 0;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;

    rc = MQTT_SUCCESS;
    create_opts.sendWhileDisconnected = 1;
    create_opts.maxBufferedMessages = 600;
    if (MQTTAsync_createWithOptions(c->client, c->broker->mqtthostaddr, c->broker->mqttclientid,
	    MQTTCLIENT_PERSISTENCE_NONE, NULL, &create_opts) != MQTTASYNC_SUCCESS) {
	snprintf(buf, sizeof (buf), "Could not create MQTT Client at %s uid %s password: %s",
		c->broker->mqtthostaddr, c->broker->mqttuid, c->broker->mqttpasswd);
	WriteDBGLog(buf);
	c->connected = 0;
	rc = MQTT_FAILURE;
    } else {
	MQTTAsync_setCallbacks(*c->client, context, onConnlost, onMsgArrvd, NULL);
	MQTTAsync_setConnected(*c->client, context, onReconnect);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.context = context;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.automaticReconnect = 1;
	if (c->broker->mqttpasswd != 0) conn_opts.password = c->broker->mqttpasswd;
	if (c->broker->mqttuid != 0) conn_opts.username = c->broker->mqttuid;

	WriteDBGLog("Attempting to connect");
	if ((rc = (MQTTAsync_connect(*c->client, &conn_opts))) != MQTTASYNC_SUCCESS) {
	    snprintf(buf, sizeof (buf),
		    "Failed to start connect user: %s, password: %s return code %d",
		    conn_opts.username, conn_opts.password, rc);
	    WriteDBGLog(buf);
	    exit(MQTT_FAILURE);
	}
	rc = MQTT_SUCCESS;
    }
    return rc;
}