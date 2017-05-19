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

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        int FD; //file descriptor for port Used for writing to port
        FILE *FH; //file handle for port use for reading from port
        char path[128]; // fully qualified path to device
        char id[64]; // id of device
        char topic[128]; // fully qualified topic
    } raven_t;

    typedef struct {
        float demand; // demand after applying scaling
        uint timestamp; //time
        char macid[256]; //mac ID of the RAVEn read from XML
    } raven_data_t;

    extern int RAVEn_sendCmd(raven_t, const char *);
    extern raven_t RAVEn_create(const char *, const char *, const char *);
    extern int RAVEn_openPort(raven_t *);
    extern void RAVEn_closePort(raven_t);
    extern int RAVEn_getData(raven_t, raven_data_t *);

#ifdef __cplusplus
}
#endif

#endif /* RAVEN_H */

