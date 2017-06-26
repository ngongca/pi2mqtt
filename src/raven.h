/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   raven.h
 * Author: nick
 *
 * Created on April 8, 2017, 11:03 PM
 */

#ifndef RAVEN_H
#define RAVEN_H

#ifndef RAVEN_PASS
#define RAVEN_PASS 0
#endif

#ifndef RAVEN_FAIL
#define RAVEN_FAIL -1
#endif

#include "mqtt.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        int FD; //file descriptor for port Used for writing to port
        FILE *FH; //file handle for port use for reading from port
        char path[128]; // fully qualified path to device
        char id[64]; // id of device
        char location[64]; ///< value of location for topic.  prefer non spaces
        char topic[64]; ///< final layer of topic of data sent. 
    } raven_t;

    typedef struct {
        float demand; // demand after applying scaling
        uint timestamp; //time
        char macid[256]; //mac ID of the RAVEn read from XML
    } raven_data_t;

    /**
     * Send command to RAVEn device
     * @param rvn - device descriptor
     * @param _cmd - command to send
     * @return - returns RAVEN_PASS if successful, RAVEN_FAIL if not.
     */
    extern int RAVEn_sendCmd(raven_t rvn, const char* cmd);
    extern int ProcessRAVEnData(raven_t rvn, mqtt_data_t* message);
    /**
     * \brief Create a RAVEn port
     * 
     * Initializes the RAVEn port data.  
     * 
     * @param path location of serial port that RAVEn is connected
     * @param id generic ID of the RAVEn for topic
     * @param topic string to append at the tail of the RAVEn topic
     * @param location used for identifying where this RAVEn port is in the topic
     * @return the newly created RAVEn port structure.  see @raven_t
     */
    extern raven_t RAVEn_create(const char* path, const char* id, const char* topic, const char* location);
    /**
     * Open the RAVEn port for read and write access
     * @param rvn
     * @return RAVEN_PASS if successful, RAVEN_FAIL if not.
     */
    extern int RAVEn_openPort(raven_t* rvn);
    
    /**
     * Close the RAVEn port.
     * @param rvn port to close
     */
    extern void RAVEn_closePort(raven_t rvn);

#ifdef __cplusplus
}
#endif

#endif /* RAVEN_H */

