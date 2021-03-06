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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include "raven.h"
#include "debug.h"
#include "mqtt.h"

static char xmlBuf[10 * 1024];

int
RAVEn_sendCmd(raven_t rvn, char const *cmd) {
    int len;
    char buf[256];
    char cmdbuffer[1024];
    int cmdlen;

    if (strlen(cmd) > sizeof (cmdbuffer))
        return (RAVEN_FAIL);
    cmdlen = snprintf(cmdbuffer, sizeof (cmdbuffer), "<Command>\r\n  <Name>%s</Name>\r\n</Command>\r\n", cmd);
    len = write(rvn.FD, cmdbuffer, cmdlen);
    if (len != cmdlen) {
        snprintf(buf, sizeof (buf), "RAVEn: Error writing Command to RAVEn port, length %d != %d", len, cmdlen);
        WriteDBGLog(buf);
        return ( RAVEN_FAIL);
    }
    return ( RAVEN_PASS);
}

raven_t
RAVEn_create(const char *path, const char *id, const char *topic, const char *location) {
    raven_t raven;
    strncpy(raven.path, path, sizeof (raven.path));
    strncpy(raven.id, id, sizeof (raven.id));
    strncpy(raven.topic, topic, sizeof (raven.topic));
    strncpy(raven.location, location, sizeof (raven.location));
    return raven;
}

int
RAVEn_openPort(raven_t *rvn) {
    char buf[128];

    snprintf(buf, sizeof (buf), "RAVEn: Opening port [%s]", rvn->path);
    WriteDBGLog(buf);
    rvn->FD = open(rvn->path, O_RDWR);
    fcntl(rvn->FD, F_SETFL, FNDELAY); // Set to non blocking.
    if (rvn->FD == -1) // if open is unsuccessful
    {
        snprintf(buf, sizeof (buf), "RAVEn open_port: Unable to open %s. 0x%0x - %s\n", rvn->path, errno, strerror(errno));
        WriteDBGLog(buf);
        perror(buf);
        return ( RAVEN_FAIL);
    } else {
        rvn->FH = fdopen(rvn->FD, "r");
        if (rvn->FH == NULL) {
            snprintf(buf, sizeof (buf), "RAVEn open_port: Unable to open %s. 0x%0x - %s\n", rvn->path, errno, strerror(errno));
            WriteDBGLog(buf);
            perror(buf);
            return ( RAVEN_FAIL);
        }
        WriteDBGLog("RAVEn: Port is open, both File Descriptor and File Handle");
    }
    return (RAVEN_PASS);
} //open_port

void
RAVEn_closePort(raven_t rvn) {
    WriteDBGLog("RAVEn: Closing Port");
    fclose(rvn.FH);
    close(rvn.FD);
}

/**
 * 
 * @param buffer
 * @param data_ptr
 * @return RAVEN_PASS if there was a match and data is good, RAVEN_FAIL otherwise.
 */
static int
RAVEn_parseXML(char buffer[], raven_data_t *data_ptr) {
    char* p;
    int demand;
    uint demand_u;
    uint multiplier;
    uint divisor;
    uint timestamp;
    char buf[128];

    p = strtok(buffer, "<>\n");
    if (strcmp(p, "InstantaneousDemand") == 0) {
        p = strtok(NULL, "<>\n");
        while (p != NULL) {
            if (strcmp(p, "Demand") == 0) {
                sscanf(strtok(NULL, "<>\n"), "0x%x", &demand_u);
            } else if (strcmp(p, "DeviceMacId") == 0) {
                strncpy(data_ptr->macid, strtok(NULL, "<>\n"), sizeof (data_ptr->macid));
            } else if (strcmp(p, "Multiplier") == 0) {
                sscanf(strtok(NULL, "<>\n"), "0x%x", &multiplier);
                if (multiplier == 0) multiplier = 1;
            } else if (strcmp(p, "Divisor") == 0) {
                sscanf(strtok(NULL, "<>\n"), "0x%x", &divisor);
                if (divisor == 0) divisor = 1;
            } else if (strcmp(p, "TimeStamp") == 0) {
                sscanf(strtok(NULL, "<>\n"), "0x%x", &timestamp);
                data_ptr->timestamp = timestamp + 946684806;
            }
            p = strtok(NULL, "<>\n");
        }
        demand = demand_u;
        if (demand >= 2^23) demand = demand - 2^24;
        snprintf(buf, sizeof (buf), "demandu 0x%x demand %d, multiplier %d, divisor %d", demand_u, demand, multiplier, divisor);
        WriteDBGLog(buf);
        data_ptr->demand = (double) demand * (double) multiplier / (double) divisor;
        return RAVEN_PASS;
    } else {
        return RAVEN_FAIL;
    }
}

int
ProcessRAVEnData(raven_t rvn, mqtt_data_t *message) {
    int retval;
    int rblen;
    char readBuf[32];

    int xmlBufLen;
    raven_data_t rvnData;

    retval = RAVEN_FAIL;
    xmlBufLen = 0;
    memset(readBuf, 0, sizeof ( readBuf));
    while (fgets(readBuf, sizeof (readBuf), rvn.FH) > 0) {
        rblen = strlen(readBuf);
        /* If Current buffer size + new Buffer being added is over the total buffer size BAD overflow */
        if ((xmlBufLen + rblen) > 10 * 1024) {
            WriteDBGLog("RAVEn: Error BUFFER OVERFLOW");
            WriteDBGLog(xmlBuf);
            break;
        } else {
            strncat(xmlBuf, readBuf, sizeof (xmlBuf));
            xmlBufLen += rblen;
            /* Check if this is the final XML tag Ending */
            /* Since I'm not really parsing XML, I am assuming the Rainforest dongle is spitting out its specific XML */
            /* They always add two space for XML inbetween the start and stop So the closing XML will always be </    */
            if (strncmp(readBuf, "</", 2) == 0) {
                WriteDBGLog("Starting to PROCESS RAVEn input");
                //                WriteDBGLog(xmlBuf);
                if (RAVEn_parseXML(xmlBuf, &rvnData) == RAVEN_PASS) {
                    snprintf(message->payload, sizeof (message->payload), "{\"timestamp\":%u,\"value\":%.3f}", time(NULL), rvnData.demand);
                    snprintf(message->topic, sizeof (message->topic), "%s/%s/%s", rvn.id, rvn.location, rvn.topic);
                    retval = RAVEN_PASS;
                    break;
                }
                memset(xmlBuf, 0, sizeof (xmlBuf));
            }
        }
    }
    return retval;
}

