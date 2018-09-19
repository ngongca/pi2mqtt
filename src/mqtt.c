/*
 * The MIT License
 *
 * Copyright 2017 Nick Ong <nick@ongsend.com>.
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
#include <errno.h>
#include "mqtt.h"
#include "debug.h"

#define QOS          1
#define TIMEOUT      10000L

static char* dumpFilename = MQTT_DUMP_FILE;

static void
onConnectFailure(void* context, MQTTAsync_failureData* response) {
    char buf[256];
    my_context_t *c = (my_context_t *) context;
    snprintf(buf, sizeof (buf), "Connect failed, rc %d", response ? response->code : 0);
    WriteDBGLog(buf);
}

static void
onConnect(void* context, MQTTAsync_successData* response) {
    my_context_t *c = (my_context_t *) context;
    mqtt_data_t data;

    c->connected = 1;
    WriteDBGLog("Successful connection");
    mqttSub(c, c->broker->mqttmanagementtopic);

    snprintf(data.payload, sizeof (data.payload),
	    "{\"timestamp\":%ld,\"status\":\"connected\"}", time(NULL));
    snprintf(data.topic, sizeof (data.topic), "%s", "manage");
    mqttPublish(c, &data);

}

void
onSubscribe(void* context, MQTTAsync_successData* response) {
    WriteDBGLog("Subscribe succeeded\n");
}

static void
onReconnect(void* context, char* response) {
    my_context_t *c = (my_context_t *) context;
    FILE *fp;
    char buf[1024];
    mqtt_data_t data;

    WriteDBGLog("onReconnect - entry");

    // connected = 0 is first time through, not a reconnect.
    if (c->connected == 0) {
	c->connected = 1;
	WriteDBGLog("onReconnect - Successful reconnection");
	mqttSub(c, c->broker->mqttmanagementtopic);
	if ((fp = fopen(dumpFilename, "r")) != NULL) {
	    while (fgets(buf, sizeof (buf), fp) != NULL) {
		sscanf(buf, "%s\n", data.topic);
		fgets(buf, sizeof (buf), fp);
		sscanf(buf, "%s\n", data.payload);
		snprintf(buf, sizeof (buf), "publishing %s to %s", data.payload, data.topic);
		WriteDBGLog(buf);
		mqttPublish(c, &data);
	    }
	    fclose(fp);
	} else {
	    perror("mqtt.c->onReconnect");
	}
	snprintf(data.payload, sizeof (data.payload),
		"{\"timestamp\":%ld,\"connection\":\"reconnected\"}", time(NULL));
	snprintf(data.topic, sizeof (data.topic), "%s", "manage");
	mqttPublish(c, &data);
    }
}

static void
onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    char buf[128];
    my_context_t *c = (my_context_t *) context;
    snprintf(buf, sizeof (buf), "Subscribe failed, rc %d", response ? response->code : 0);
    WriteDBGLog(buf);
    c->killed = 1;
}

static void
onConnLost(void *context, char *cause) {
    my_context_t* c = (my_context_t *) context;
    FILE *fp;
    char buf[1024];
    mqtt_data_t data;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    c->connected = 0;
    snprintf(buf, 64, "onConnlost - Broker connection lost. Cause: %s", cause);
    WriteDBGLog(buf);
    if ((fp = fopen(dumpFilename, "w")) == NULL) {
	perror("mqtt.c->onConnLost");
    } else {
	fclose(fp);
    }

    // Let the broker know when the connection was lost
    snprintf(data.payload, sizeof (data.payload),
	    "{\"timestamp\":%ld,\"connection\":\"lost\"}", time(NULL));
    snprintf(data.topic, sizeof (data.topic), "%s", "manage");
    mqttPublish(c, &data);

}

/**
 * 
 * @param context
 * @param topicName
 * @param topicLen
 * @param message
 * @return 
 */
static int
onMsgArrvd(void *context, char* topicName, int topicLen, MQTTAsync_message *message) {
    char buf[128];
    FILE *fp;
    my_context_t *c = (my_context_t *) context;
    int i;
    char* payloadptr;
    mqtt_data_t data;

    snprintf(buf, sizeof (buf), "onMsgArrvd - Message arrived on topic: %s", topicName);
    WriteDBGLog(buf);

    char *token = strtok(topicName, "/");
    char *key;
    while (token != NULL) {
	key = token;
	token = strtok(NULL, "/");
    }

    if (strcmp(key, "kill") == 0) {
	c->killed = 1;
	WriteDBGLog("onMsgArrvd - pi2mqtt killed");
	snprintf(data.payload, sizeof (data.payload),
		"{\"timestamp\":%ld,\"system\":\"kill requested\"}", time(NULL));
	snprintf(data.topic, sizeof (data.topic), "%s", "manage");
	mqttPublish(c, &data);
    }

    if (strcmp(key, "update") == 0) {
	snprintf(buf, sizeof (buf), "%s.bak", c->configFile);
	if (rename(c->configFile, buf) == 0) {
	    if ((fp = fopen(c->configFile, "w")) != NULL) {
		payloadptr = message->payload;
		for (i = 0; i < message->payloadlen; i++) {
		    fputc(*payloadptr, fp);
		    payloadptr++;
		}
		fclose(fp);
		c->reboot = 1;
		WriteDBGLog("onMsgArrvd - updated configuration file");
		snprintf(data.payload, sizeof (data.payload),
			"{\"timestamp\":%ld,\"system\":\"update\"}", time(NULL));
		snprintf(data.topic, sizeof (data.topic), "%s", "manage");
		mqttPublish(c, &data);
	    } else {
		perror("mqtt.c->onMsgArrvd");
	    }
	} else {
	    WriteDBGLog("onMsgArrvd - unable to backup configFile");
	}
    }

    if (strcmp(key, "reboot") == 0) {
	c->reboot = 1;
	WriteDBGLog("onMsgArrvd - reboot requested");
	snprintf(data.payload, sizeof (data.payload),
		"{\"timestamp\":%ld,\"system\":\"reboot requested\"}", time(NULL));
	snprintf(data.topic, sizeof (data.topic), "%s", "manage");
	mqttPublish(c, &data);
    }

    if (strcmp(key, "read") == 0) {
	c->readData = 1;
	WriteDBGLog("onMsgArrvd - instant read requested");
	snprintf(data.payload, sizeof (data.payload),
		"{\"timestamp\":%ld,\"system\":\"read requested\"}", time(NULL));
	snprintf(data.topic, sizeof (data.topic), "%s", "manage");
	mqttPublish(c, &data);
    }
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void
onSend(void *context, MQTTAsync_successData* response) {
    my_context_t *c = (my_context_t *) context;
    char buf[128];
    snprintf(buf, sizeof (buf), "onSend - Message with token value %d delivery confirmed", response->token);
    WriteDBGLog(buf);
}

void
mqttSave(void *context, const mqtt_data_t msg) {
    my_context_t *c = (my_context_t *) context;
    FILE *fp;
    char buf[256];

    if ((fp = fopen(dumpFilename, "a")) == NULL) {
	snprintf(buf, sizeof (buf), "mqttSave - error opening persistence file %s", dumpFilename);
	WriteDBGLog(buf);
	perror("mqtt.c->mqttSave");
	return;
    }
    snprintf(buf, sizeof (buf), "mqttSave - Saving topic %s/%s message %s to file", c->broker->mqtthome, msg.topic, msg.payload);
    WriteDBGLog(buf);
    fprintf(fp, "%s\n", msg.topic);
    fprintf(fp, "%s\n", msg.payload);
    fclose(fp);
}

void
mqttSub(void *context, const char *topic) {
    char buf[512];
    my_context_t *c = (my_context_t *) context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc;
    if (c->connected == 1) {
	snprintf(buf, sizeof (buf), "mqttSub - Subscribing to topic %s for client %s using QoS %d",
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
mqttPublish(void *context, mqtt_data_t *message) {
    MQTTAsync_token token;
    my_context_t *c = (my_context_t *) context;
    char buf[256];
    snprintf(buf, sizeof (buf), "mqttPublish - %s to %s/%s Connected %d", message->payload, 
	    c->broker->mqtthome, message->topic, c->connected);
    WriteDBGLog(buf);
    if (c->connected == 1) {
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

	opts.onSuccess = onSend;
	opts.context = c;
	pubmsg.payload = message->payload;
	pubmsg.payloadlen = strlen(message->payload);
	pubmsg.qos = QOS;
	pubmsg.retained = 1;
	token = 0;

	snprintf(buf, sizeof (buf), "%s/%s", c->broker->mqtthome, message->topic);
	if ((token = MQTTAsync_sendMessage(*c->client, buf, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
	    snprintf(buf, sizeof (buf), "mqttPublish - Failed to start sendMessage, return code %d\n", token);
	    WriteDBGLog(buf);
	    c->killed = 1;
	    return (MQTT_FAILURE);
	}

    } else {
	
	mqttSave(c, *message);
    }
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
    MQTTAsync_willOptions lwt_opts = MQTTAsync_willOptions_initializer;

    rc = MQTT_SUCCESS;
    snprintf(buf, sizeof (buf), "MQTT_init - create MQTT Client at %s uid %s password: %s",
	    c->broker->mqtthostaddr, c->broker->mqttuid, c->broker->mqttpasswd);
    WriteDBGLog(buf);
    if (MQTTAsync_create(c->client, c->broker->mqtthostaddr, c->broker->mqttclientid,
	    MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTASYNC_SUCCESS) {
	snprintf(buf, sizeof (buf), "Could not create MQTT Client at %s uid %s password: %s",
		c->broker->mqtthostaddr, c->broker->mqttuid, c->broker->mqttpasswd);
	WriteDBGLog(buf);
	c->connected = 0;
	rc = MQTT_FAILURE;
    } else {
	char top[256];
	lwt_opts.message = "{\"status\":\"system offline\"}";
	snprintf(top, 256, "%s/manage", c->broker->mqtthome);
	lwt_opts.topicName = top;
	lwt_opts.qos = 1;
	lwt_opts.retained = 1;

	MQTTAsync_setCallbacks(*c->client, context, onConnLost, onMsgArrvd, NULL);
	MQTTAsync_setConnected(*c->client, context, onReconnect);
	
	conn_opts.will = &lwt_opts;
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.context = context;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.automaticReconnect = 1;
	
	if (c->broker->mqttpasswd != 0) conn_opts.password = c->broker->mqttpasswd;
	if (c->broker->mqttuid != 0) conn_opts.username = c->broker->mqttuid;
	snprintf(buf, sizeof (buf), "MQTT_init - Attempting to connect to %s %s", conn_opts.username, conn_opts.password);
	WriteDBGLog(buf);
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