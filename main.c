/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: nick
 *
 * Created on April 1, 2017, 1:56 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include "debug.h"
#include <errno.h>
#include <err.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "MQTTClient.h"
#include "confuse.h"
#include "ds18b20pi.h"
#include "raven.h"

#define STARTUP "\n\npi2mqtt - \nVersion 0.1 Mar 28, 2017\nRead sensors from RPI and publish data to mqtt\r\n\r\n"
char *gApp = "pi2mqtt";

char gTempDevLoc[128];
char gTempDevPath[256];

char gCmdBuffer[1024 * 5];
int gCmdBufferLen;

#define QOS          1
#define TIMEOUT      10000L

#define MQTTPASSWORD "piiot"
#define MQTTUSER "mqttpi"

//----------------------------------------
//----------------------------------------

typedef struct {
    int killed; // flag to kill loop
} my_context_t;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    char buf[128];
    snprintf(buf, sizeof (buf), "Message with token value %d delivery confirmed", dt);
    WriteDBGLog(buf);


}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
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

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

cfg_t *read_config(char config_filename[]) {

    cfg_t *cfg;
    static cfg_opt_t ds18b20_opts[] = {
        CFG_STR("address", "/sys/bus/w1/devices", CFGF_NONE),
        CFG_STR("mqttpubtopic", "home/device/temp", CFGF_NONE),
        CFG_INT("isfahrenheit", 1, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t raven_opts[] = {
        CFG_STR("address", "/dev/ttyUSB0", CFGF_NONE),
        CFG_STR("mqttpubtopic", "home/device/demand", CFGF_NONE),
        CFG_END()
    };

    cfg_opt_t opts[] = {
        CFG_INT("delay", 5, CFGF_NONE),
        CFG_STR("mqttbrokeraddress", "localhost", CFGF_NONE),
        CFG_STR("mqttsubtopic", "home/+/manage", CFGF_NONE),
        CFG_STR("clientid", "id", CFGF_NONE),
        CFG_STR("debuglogfile", "./debug.log", CFGF_NONE),
        CFG_INT("debugmode", 0, CFGF_NONE),
        CFG_SEC("ds18b20", ds18b20_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC("RAVEn", raven_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_END()
    };

    cfg = cfg_init(opts, 0);
    if (cfg_parse(cfg, config_filename) != CFG_SUCCESS) {
        errx(1, "Failed parsing configuration!\n");
    }
    return (cfg);
}

/**
 * Load the initial parameters.
 * TODO: build a parser to read XML file at setup.
 */
void LoadINIParms(cfg_t **config, DS18B20PI_port_t *sensor, raven_t *raven, MQTTClient *client, char configFile[]) {

    int rc = 0;
    int i;
    cfg_t *scfg;
    char buf[64];

    *config = read_config(configFile);

    for (i = 0; i < cfg_size(*config, "ds18b20"); i++) {
        // TODO: create multiple sensors.  For now just reading the last entry.
        scfg = cfg_getnsec(*config, "ds18b20", i);
        DS18B20PI_initPort(sensor, cfg_getstr(scfg, "address"), cfg_title(scfg), cfg_getstr(scfg, "mqttpubtopic"), cfg_getint(scfg, "isfahrenheit"));
    }
    for (i = 0; i < cfg_size(*config, "RAVEn"); i++) {
        // TODO: create multiple sensors.  For now just reading the last entry.
        scfg = cfg_getnsec(*config, "RAVEn", i);
        RAVEn_init(raven, cfg_getstr(scfg, "address"), cfg_title(scfg), cfg_getstr(scfg, "mqttpubtopic"));
    }
    if (MQTTClient_create(client, cfg_getstr(*config, "mqttbrokeraddress"), cfg_getstr(*config, "clientid"), MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS) {
        DS18B20PI_closePort(*sensor);
        snprintf(buf, sizeof (buf), "Could not create MQTT Client at %s", cfg_getstr(*config, "mqttbrokeraddress"));
        exit(-1);
    }
}

int mqttSendData(MQTTClient mqttClient, char *payload, char *topic) {
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
    snprintf(dbgBuf, sizeof (dbgBuf), "Waiting for up to %d seconds for publication of \n%s\non topic %s",
            (int) (TIMEOUT / 1000), payload, topic);
    WriteDBGLog(dbgBuf);
    rc = MQTTClient_waitForCompletion(mqttClient, token, TIMEOUT);
    snprintf(dbgBuf, sizeof (dbgBuf), "Message with delivery token %d delivered", token);
    WriteDBGLog(dbgBuf);
}

/*===========================================================================*/
/* PROCESSDATA - Process the usb com port and store XML data into SQL DB     */

/*===========================================================================*/
int ProcessDS18B20PIData(DS18B20PI_port_t sensorPort, MQTTClient mqttClient) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    int rc;
    DS18B20PI_data_t data;

    char mqttPayload[1024];
    char dbgBuf[1024];

    rc = 0;
    WriteDBGLog("Starting to PROCESS input");
    if ((rc = DS18B20PI_getSensorTemp(sensorPort, &data)) == DS18B20PI_SUCCESS) {
        snprintf(mqttPayload, sizeof (mqttPayload), "{\"temperature\":{\"timestamp\":%d,\"value\":%.3f}}", data.timestamp, data.temperature);
        snprintf(dbgBuf, sizeof (dbgBuf), "Sending payload %s", mqttPayload);
        WriteDBGLog(dbgBuf);
        rc = mqttSendData(mqttClient, mqttPayload, sensorPort.topic);
    } else {
        WriteDBGLog("Failed to read temperature sensor");
        rc = 0;
    }
    return (rc);
}

int ProcessRAVEnData(raven_t rvn, MQTTClient mqttClient) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    raven_data_t data;

    char mqttPayload[1024];
    char dbgBuf[1024];

    WriteDBGLog("Starting to PROCESS RAVEn input");
    if ((RAVEn_getData(rvn, &data)) == RAVEN_PASS) {
        snprintf(mqttPayload, sizeof (mqttPayload), "{\"demand\":{\"timestamp\":%d,\"value\":%.3f}}", data.timestamp, data.demand);
        snprintf(dbgBuf, sizeof (dbgBuf), "Sending payload %s", mqttPayload);
        WriteDBGLog(dbgBuf);
        mqttSendData(mqttClient, mqttPayload, rvn.topic);
    } else {
        WriteDBGLog("Failed to read RAVEn sensor");
        return (RAVEN_FAIL);
    }
    return (RAVEN_PASS);
}

/*
 * 
 */
int main(int argc, char** argv) {
    int c;
    int rc;
    char buf[1024];
    int cntr;
    DS18B20PI_port_t sensorPort;
    raven_t ravenPort;
    MQTTClient mqttClient;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    my_context_t my_context;
    cfg_t *cfg = 0;
    int verbose = 0;
    char *configFile = "./ds18b20pi.config";

    while ((c = getopt(argc, argv, "v?hc:")) != -1) {
        switch (c) {
            case 'v':
                verbose = 1;
                break;
            case '?':
            case 'h':
                printf(STARTUP);
                printf("\r\n-v - VERBOSE print everything that would go to DEBUG LOG if debug was turned on\r\n");
                printf("\r\n-c <filename> - Configuration file\r\n");
                exit(EXIT_SUCCESS);
                break;
            case 'c':
                configFile = optarg;
                break;
            default:
                printf("? Unrecognizable switch [%s] - program aborted\n", optarg);
                exit(-1);
        }
    }

    LoadINIParms(&cfg, &sensorPort, &ravenPort, &mqttClient, configFile); // Initialize ports.
    InitDBGLog("t2MQTT", cfg_getstr(cfg, "debuglogfile"), cfg_getint(cfg, "debugmode"), verbose);
    WriteDBGLog(STARTUP);
    rc = 0;
    my_context.killed = 0;
    MQTTClient_setCallbacks(mqttClient, &my_context, connlost, msgarrvd, delivered);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.password = MQTTPASSWORD;
    conn_opts.username = MQTTUSER;
    if ((rc = MQTTClient_connect(mqttClient, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        snprintf(buf, sizeof (buf), "Failed to connect, return code %d", rc);
        WriteDBGLog(buf);
        exit(EXIT_FAILURE);
    }
    snprintf(buf, sizeof (buf), "Subscribing to topic %s\nfor client %s using QoS%d", cfg_getstr(cfg, "mqttsubtopic"), cfg_getstr(cfg, "clientid"), QOS);
    WriteDBGLog(buf);
    MQTTClient_subscribe(mqttClient, cfg_getstr(cfg, "mqttsubtopic"), QOS);
    cntr = 0;
    if (RAVEn_openPort(&ravenPort) != RAVEN_PASS) {
        WriteDBGLog("Error opening RAVEn Port");
        exit(EXIT_FAILURE);
    }
    RAVEn_sendCmd(ravenPort, "initialize");

    while (!my_context.killed) {
        if (cfg_getint(cfg, "delay") > cntr) {
            // must reopen port to get a reading.
            if (DS18B20PI_openPort(&sensorPort) != DS18B20PI_SUCCESS) {
                WriteDBGLog("Error opening DS18B20 port");
                exit(EXIT_FAILURE);
            }
            ProcessDS18B20PIData(sensorPort, mqttClient);
            cntr = 0;
            DS18B20PI_closePort(sensorPort);
        }
        ProcessRAVEnData(ravenPort, mqttClient);
        cntr++;
        sleep(1);
    }
    RAVEn_closePort(ravenPort);


    WriteDBGLog("Closing mqttClient");
    MQTTClient_disconnect(mqttClient, 10000);
    MQTTClient_destroy(&mqttClient);
    return (rc);
}
