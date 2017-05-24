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
#include <errno.h>
#include <fcntl.h>
#include "raven.h"

/**
 * Send command to RAVEn device
 * @param rvn - device descriptor
 * @param _cmd - command to send
 * @return - returns RAVEN_PASS if successful, RAVEN_FAIL if not.
 */
int 
RAVEn_sendCmd(raven_t rvn, char const *cmd)
{
    int len;
    char buf[256];
    char cmdbuffer[1024];
    int cmdlen;

    if (strlen(cmd) > sizeof (cmdbuffer))
        return (RAVEN_FAIL);
    cmdlen = snprintf(cmdbuffer, sizeof (cmdbuffer), "<Command>\r\n  <Name>%s</Name>\r\n</Command>\r\n", cmd);
    WriteDBGLog(cmdbuffer);
    len = write(rvn.FD, cmdbuffer, cmdlen);
    if (len != cmdlen) {
        snprintf(buf, sizeof (buf), "RAVEn: Error writing Command to RAVEn port, length %d != %d", len, cmdlen);
        WriteDBGLog(buf);
        return ( RAVEN_FAIL);
    }
    return ( RAVEN_PASS);
}

/**
 * Initialize the raven_t descriptor
 * @param raven_ptr pointer to the descriptor to initialize
 * @param path full path to the location of the device
 * @param id name for identification
 * @param topic default publish topic for this descriptor
 */
raven_t 
RAVEn_create(const char *path, const char *id, const char *topic) 
{
    raven_t raven;
    strncpy(raven.path, path, sizeof (raven.path));
    strncpy(raven.id, id, sizeof (raven.id));
    strncpy(raven.topic, topic, sizeof (raven.topic));
    return raven;
}

/**
 * Open the RAVEn port for read and write access
 * @param raven_ptr
 * @return RAVEN_PASS if successful, RAVEN_FAIL if not.
 */
int 
RAVEn_openPort(raven_t *raven_ptr)
{
    char buf[128];

    snprintf(buf, sizeof (buf), "RAVEn: Opening port [%s]", raven_ptr->path);
    WriteDBGLog(buf);
    raven_ptr->FD = open(raven_ptr->path, O_RDWR);
    if (raven_ptr->FD == -1) // if open is unsucessful
    {
        snprintf(buf, sizeof (buf), "RAVEn open_port: Unable to open %s. 0x%0x - %s\n", raven_ptr->path, errno, strerror(errno));
        WriteDBGLog(buf);
        perror(buf);
        return ( RAVEN_FAIL);
    } else {
        raven_ptr->FH = fdopen(raven_ptr->FD, "r");
        if (raven_ptr->FH == NULL) {
            snprintf(buf, sizeof (buf), "RAVEn open_port: Unable to open %s. 0x%0x - %s\n", raven_ptr->path, errno, strerror(errno));
            WriteDBGLog(buf);
            perror(buf);
            return ( RAVEN_FAIL);
        }
        WriteDBGLog("RAVEn: Port is open, both File Descriptor and File Handle");
    }
    return (RAVEN_PASS);
} //open_port

void 
RAVEn_closePort(raven_t rvn) 
{
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
RAVEn_parseXML(char buffer[], raven_data_t *data_ptr) 
{
    char* p;
    int demand;
    uint demand_u;
    uint multiplier;
    uint divisor;
    uint timestamp;

    p = strtok(buffer, "<>\n");
    if (strcmp(p, "InstantaneousDemand") == 0) {
        p = strtok(NULL, "<>\n");
        while (p != NULL) {
            if (strcmp(p, "Demand") == 0) {
                sscanf(strtok(NULL, "<>\n"), "0x%x", &demand_u);
                demand = demand_u;
                if (demand >= 2^23) demand = demand - 2^24;
            } else if (strcmp(p, "DeviceMacId") == 0) {
                strncpy(data_ptr->macid, strtok(NULL, "<>\n"), sizeof(data_ptr->macid));
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
        data_ptr->demand = (double) demand * (double) multiplier / (double) divisor;
        return RAVEN_PASS;
    } else {
        return RAVEN_FAIL;
    }
}


/**
 * RAVEn_getData - Read data from RAVEN port
 * @param rvn port to read
 * @param rvnData_ptr address to place data
 * @return 
 */
int 
RAVEn_getData(raven_t rvn, raven_data_t *rvnData_ptr) 
{
    int retval;
    int rblen;
    char readBuf[1024];
    char xmlBuf[10*1024];
    int xmlBufLen;
    
    retval = RAVEN_FAIL;
    WriteDBGLog("RAVEn: Starting to PROCESS input");
    xmlBufLen=0;
    memset(readBuf, 0, sizeof ( readBuf));
    while ((fgets(readBuf, sizeof ( readBuf), rvn.FH)) > 0) {
        rblen = strlen(readBuf);
        /* If Current buffer size + new Buffer being added is over the total buffer size BAD overflow */
        if ((xmlBufLen + rblen) > 10*1024) {
            WriteDBGLog("RAVEn: Error BUFFER OVERFLOW");
            WriteDBGLog(xmlBuf);
            WriteDBGLog(readBuf);
            break;
        } else {
            strncat(xmlBuf, readBuf, sizeof(xmlBuf));
            xmlBufLen += rblen;
            /* Check if this is the final XML tag Ending */
            /* Since I'm not really parsing XML, I am assuming the Rainforest dongle is spitting out its specific XML */
            /* They always add two space for XML inbetween the start and stop So the closing XML will always be </    */
            if (strncmp(readBuf, "</", 2) == 0) {
                retval = RAVEn_parseXML(xmlBuf, rvnData_ptr);      
                // Write to the LOG file if its turned on
                WriteDBGLog(xmlBuf);
                memset(xmlBuf, 0, sizeof (xmlBuf));
                break;
            }
        }
        memset(readBuf, 0, sizeof ( readBuf));
    }
    return retval;
}