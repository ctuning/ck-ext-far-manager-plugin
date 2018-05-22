#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"

static FILE *LogStream=0;

FILE * OpenLogStream(char *file)
{

    char rfile[260];

    strcpy(rfile,file);

    return fopen(rfile,"a+t");
}

void OpenSysLog(char *file)
{
    if ( LogStream )
        fclose(LogStream);

    LogStream=OpenLogStream(file);
    if ( !LogStream )
    {
        fprintf(stderr,"Can't open log file '%s'\n",file);
    }
}

void CloseSysLog(void)
{
    fclose(LogStream);
    LogStream=0;
}

void SysLog(char *fmt,...)
{
    char msg[MAX_LOG_LINE];
    time_t t;
    struct tm *tm;
    char timebuf[64];
    FILE *std;
    char suffix[4];

    va_list argptr;
    va_start( argptr, fmt );

    vsprintf( msg, fmt, argptr );
    va_end(argptr);

    suffix[3]=0;
    strncpy(suffix,msg,3);
    time (&t);
    tm = localtime (&t);
    strftime (timebuf, sizeof (timebuf), "%d.%m.%Y %H:%M:%S", tm);

    OpenSysLog("d:\\me-like.log");
    fprintf(LogStream,"%s %s\n",timebuf,msg);
    fflush(LogStream);
    CloseSysLog();
}

