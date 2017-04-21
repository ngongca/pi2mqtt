/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* ======================================================================================================== */
/* DBGLOG.C - Write to a Debug LOG file																		*/
/*																											*/
/* Inital Author: Andy Wysocki   03/16/2013
 * Additional Edit by: Nick Ong 03/31/2017																	*/
/*																											*/
/* This code is release under the Open Software License v. 3.0 (OSL-3.0)									*/
/*					http://opensource.org/licenses/OSL-3.0													*/
/*																											*/
/*	Release 1.0 - 03/15/2012																				*/
/*																											*/
/* ======================================================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "debug.h"


static  char    sMyKey[32];
static  char    sDebugLog[255];
static  int     sDebug;
static	int		sVerbose = 0;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void
InitDBGLog( char *_Key, char * _FileName, int _Debug, int _Verbose )
{

    strcpy( sMyKey, _Key );
    strcpy( sDebugLog, _FileName );
	sVerbose = _Verbose;
    sDebug = _Debug;

}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void
WriteDBGLog( char *_Str )
{
    FILE  *FH;
    time_t  tt;
    struct  tm      *tm;
	char	header[512];

	tt = time(NULL);
	tm = localtime(&tt);
	sprintf(header, "%s-%04d/%02d/%02d %02d:%02d:%02d ", sMyKey, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);



    if (sDebug)
    {
        FH = fopen(sDebugLog, "a+");
        if (FH == NULL)
        {
            return;
        } /* endif */

        fputs( header, FH );

        fputs( _Str, FH );
        
        if ( _Str[ strlen( _Str) - 1] != '\n' )
            fprintf( FH, "\n" );
        
        fclose(FH);
    } /* endif */

	if ( sVerbose )
		printf( "%s %s\n", header, _Str );
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void
WriteRunLog( char *_Str )
{
    FILE  *FH;
    time_t  tt;
    struct  tm      *tm;
    
    FH = fopen("./runlog", "a+");
    if (FH == NULL)
    {
        return;
    } /* endif */
    
    tt = time(NULL);
    tm = localtime(&tt);
    fprintf(FH, "%04d/%02d/%02d %02d:%02d:%02d ", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    fputs( _Str, FH );
    
    if ( _Str[ strlen( _Str) - 1] != '\n' )
        fprintf( FH, "\n" );
    
    fclose(FH);

}