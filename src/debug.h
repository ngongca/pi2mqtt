/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   debug.h
 * Author: nick
 *
 * Created on April 1, 2017, 2:03 PM
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

void InitDBGLog( char *_Key, char * _FileName, int _Debug, int _Verbose );
void WriteDBGLog( char *_Str );

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H */
