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

/**
 * File: main.c
 * @author: Nick Ong
 * @version 1.0
 * @brief Read various sensor from a Raspberry Pi and publish to MQTT
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>
#include <fcntl.h>
#include <err.h>

#include <MQTTClient.h>
#include <confuse.h>
#include "ds18b20pi.h"
#include "dht22.h"
#include "raven.h"
#include "doorswitch.h"
#include "mqtt.h"
#include "debug.h"

#define STARTUP "\n\npi2mqtt - \nVersion 0.1 Mar 28, 2017\nRead sensors from RPI and publish data to mqtt\r\n\r\n"

typedef struct {
    int size;
    DS18B20PI_port_t ports[MAXPORTS];
    long lastsample[MAXPORTS];
} ds18b20pi_ports_t;

typedef struct {
    int size;
    raven_t ports[MAXPORTS];
} raven_ports_t;

typedef struct {
    int size;
    dht22_port_t ports[MAXPORTS];
} dht22_ports_t;

typedef struct {
    int size;
    doorswitch_port_t ports[MAXPORTS];
} doorswitch_ports_t;

char *gApp = "pi2mqtt";

char gTempDevLoc[128];
char gTempDevPath[256];

char gCmdBuffer[1024 * 5];
int gCmdBufferLen;

/**
 * 
 * @param config_filename
 * @return 
 */
static cfg_t *
read_config(char config_filename[]) {
    cfg_t *cfg;
    static cfg_opt_t ds18b20_opts[] = {
        CFG_STR("address", "/sys/bus/w1/devices", CFGF_NONE),
        CFG_STR("mqttpubtopic", "temp", CFGF_NONE),
        CFG_INT("sampletime", 5, CFGF_NONE),
        CFG_STR("location", "location", CFGF_NONE),
        CFG_INT("isfahrenheit", 1, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t raven_opts[] = {
        CFG_STR("address", "/dev/ttyUSB0", CFGF_NONE),
        CFG_STR("mqttpubtopic", "home/device/demand", CFGF_NONE),
        CFG_STR("location", "location", CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t dht22_opts[] = {
        CFG_INT("pin", 1, CFGF_NONE),
        CFG_STR("mqttpubtopic", "home/device", CFGF_NONE),
        CFG_INT("isfahrenheit", 1, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t doorswitch_opts[] = {
        CFG_INT("pin", 1, CFGF_NONE),
        CFG_STR("mqttpubtopic", "home/door", CFGF_NONE),
        CFG_STR("location", "location", CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t opts[] = {
        CFG_INT("delay", 1000, CFGF_NONE),
        CFG_STR("mqttbrokeraddress", "localhost", CFGF_NONE),
        CFG_STR("mqttbrokeruid", "mqttpi", CFGF_NONE),
        CFG_STR("mqttbrokerpwd", "piiot", CFGF_NONE),
        CFG_STR("mqttsubtopic", "home/+/manage", CFGF_NONE),
        CFG_STR("clientid", "id", CFGF_NONE),
        CFG_STR("debuglogfile", "./debug.log", CFGF_NONE),
        CFG_INT("debugmode", 0, CFGF_NONE),
        CFG_SEC("ds18b20", ds18b20_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC("RAVEn", raven_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC("dht22", dht22_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC("doorswitch", doorswitch_opts, CFGF_MULTI | CFGF_TITLE),
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
static void
LoadINIParms(cfg_t **config, ds18b20pi_ports_t *sensors, raven_ports_t *raven, dht22_ports_t *dht22p,
        doorswitch_ports_t *doorswitch, mqtt_broker_t *client, char configFile[]) {
    int i;
    cfg_t *scfg;
    char buf[64];

    *config = read_config(configFile);

    for (i = 0; i < cfg_size(*config, "ds18b20"); i++) {
        if (i < MAXPORTS) {
            scfg = cfg_getnsec(*config, "ds18b20", i);
            sensors->ports[i] = DS18B20PI_createPort(cfg_getstr(scfg, "address"),
                    cfg_title(scfg), cfg_getstr(scfg, "mqttpubtopic"),
                    cfg_getint(scfg, "sampletime"),
                    cfg_getstr(scfg, "location"),
                    cfg_getint(scfg, "isfahrenheit"));
            sensors->lastsample[i] = 0;
            sensors->size++;
        }
    }
    for (i = 0; i < cfg_size(*config, "RAVEn"); i++) {
        if (i < MAXPORTS) {
            scfg = cfg_getnsec(*config, "RAVEn", i);
            raven->ports[i] = RAVEn_create(cfg_getstr(scfg, "address"), cfg_title(scfg), cfg_getstr(scfg, "mqttpubtopic"), cfg_getstr(scfg, "location"));
            raven->size++;
        }
    }
    for (i = 0; i < cfg_size(*config, "dht22"); i++) {
        if (i < MAXPORTS) {
            scfg = cfg_getnsec(*config, "dht22", i);
            dht22p->ports[i] = dht22_createPort(cfg_getint(scfg, "pin"), cfg_title(scfg), cfg_getstr(scfg, "mqttpubtopic"), cfg_getint(scfg, "isfahrenheit"));
            dht22p->size++;
        }
    }
    for (i = 0; i < cfg_size(*config, "doorswitch"); i++) {
        if (i < MAXPORTS) {
            scfg = cfg_getnsec(*config, "doorswitch", i);
            doorswitch->ports[i] = doorswitch_createPort(cfg_getint(scfg, "pin"), cfg_title(scfg), cfg_getstr(scfg, "mqttpubtopic"),
                    cfg_getstr(scfg, "location"));
            doorswitch->size++;
        }
    }
    client->mqtthostaddr = cfg_getstr(*config, "mqttbrokeraddress");
    client->mqttclientid = cfg_getstr(*config, "clientid");
    client->mqttuid = cfg_getstr(*config, "mqttbrokeruid");
    client->mqttpasswd = cfg_getstr(*config, "mqttbrokerpwd");

}

static int
ProcessDHT22Data(dht22_port_t dht22, MQTTClient mqttClient) {
    dht22_data_t data;
    mqtt_data_t mdata;
    char dbgBuf[512];

    WriteDBGLog("Starting to PROCESS dht22 input");
    if ((read_dht22_dat(dht22, &data)) == DHT22_SUCCESS) {
        snprintf(mdata.payload, sizeof (mdata.payload), "{\"temperature\":{\"timestamp\":%ld,\"value\":%.3f}}", data.timestamp, data.temperature);
        snprintf(mdata.topic, sizeof (mdata.topic), "%s/temperature", dht22.topic);
        snprintf(dbgBuf, sizeof (dbgBuf), "Topic %s Payload %s", mdata.topic, mdata.payload);
        WriteDBGLog(dbgBuf);

        MQTT_send(mqttClient, &mdata);

        snprintf(mdata.payload, sizeof (mdata.payload), "{\"humidity\":{\"timestamp\":%ld,\"value\":%.3f}}", data.timestamp, data.humidity);
        snprintf(mdata.topic, sizeof (mdata.topic), "%s/humidity", dht22.topic);
        snprintf(dbgBuf, sizeof (dbgBuf), "Topic %s Payload %s", mdata.topic, mdata.payload);
        WriteDBGLog(dbgBuf);
        MQTT_send(mqttClient, &mdata);
    } else {
        WriteDBGLog("Failed to read dht22 sensor");
        return (DHT22_FAILURE);
    }
    return (DHT22_SUCCESS);
}

int
main(int argc, char** argv) {
    int c;
    int i;
    int cntr;
    ds18b20pi_ports_t ds18b20_ports;
    raven_ports_t raven_ports;
    dht22_ports_t dht22_ports;
    doorswitch_ports_t doorswitch_ports;
    mqtt_broker_t mclient;
    MQTTClient mqtt_client;
    my_context_t my_context;
    mqtt_data_t message;
    struct timespec delay;

    delay.tv_nsec = 1000000;
    delay.tv_sec = 0;

    ds18b20_ports.size = 0;
    raven_ports.size = 0;
    dht22_ports.size = 0;
    doorswitch_ports.size = 0;

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

    LoadINIParms(&cfg, &ds18b20_ports, &raven_ports, &dht22_ports, &doorswitch_ports, &mclient, configFile); // Initialize ports.

    InitDBGLog("pi2MQTT", cfg_getstr(cfg, "debuglogfile"), cfg_getint(cfg, "debugmode"), verbose);
    WriteDBGLog(STARTUP);

    my_context.killed = 0;
    if (MQTT_init(&my_context, &mqtt_client, &mclient) == MQTT_FAILURE) {
        exit(EXIT_FAILURE);
    }
    my_context.broker = &mclient;
    my_context.client = &mqtt_client;

    MQTT_sub(mqtt_client, cfg_getstr(cfg, "mqttsubtopic"));

    cntr = 0;

    //init RAVEn
    for (i = 0; i < raven_ports.size; i++) {
        if (RAVEn_openPort(&raven_ports.ports[i]) != RAVEN_PASS) {
            WriteDBGLog("Error opening RAVEn Port");
            exit(EXIT_FAILURE);
        }
        RAVEn_sendCmd(raven_ports.ports[i], "initialize");
    }

    //init DHT22
    if (dht22_init() != DHT22_SUCCESS) {
        WriteDBGLog("Error opening DHT22 sensor");
        exit(EXIT_FAILURE);
    }
    
    if (DS18B20PI_init() != DS18B20PI_SUCCESS) {
        WriteDBGLog("Error initializing DS18B20");
        exit(EXIT_FAILURE);
    }

    if (doorswitch_init() != DOORSWITCH_SUCCESS) {
        WriteDBGLog("Error initializing Door Switches");
        exit(EXIT_FAILURE);
    }

    while (!my_context.killed) {
        for (i = 0; i < ds18b20_ports.size; i++) {
            long t = time(NULL);
            if (t - ds18b20_ports.lastsample[i] >= (long) ds18b20_ports.ports[i].sampletime) {
                ds18b20_ports.lastsample[i] = t;
                if (DS18B20PI_process_data(ds18b20_ports.ports[i], &message) == DS18B20PI_SUCCESS) {
                    MQTT_send(mqtt_client, &message);
                } else {
                    WriteDBGLog("Failed to read temperature sensor");
                }

            }
        }
        if (cfg_getint(cfg, "delay") <= cntr) {
            // process dht22
            for (i = 0; i < dht22_ports.size; i++) {
                ProcessDHT22Data(dht22_ports.ports[i], mqtt_client);
            }

            // process door switches
            for (i = 0; i < doorswitch_ports.size; i++) {
                if (ProcessDoorswitchData(&doorswitch_ports.ports[i], &message) == DOORSWITCH_SUCCESS) {
                    MQTT_send(mqtt_client, &message);
                };
            }
            cntr = 0;
        }

        for (i = 0; i < raven_ports.size; i++) {
            if (ProcessRAVEnData(raven_ports.ports[i], &message) == RAVEN_PASS) {
                MQTT_send(mqtt_client, &message);
            }
        }
        cntr++;
        nanosleep(&delay, NULL);

    }


    for (i = 0; i < raven_ports.size; i++) {
        RAVEn_closePort(raven_ports.ports[i]);
    }

    WriteDBGLog("Closing mqttClient");
    MQTTClient_disconnect(mqtt_client, 10000);
    MQTTClient_destroy(&mqtt_client);
    return (EXIT_SUCCESS);
}
