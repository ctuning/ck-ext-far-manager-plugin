#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>

void SysLog(char *msg,...);

void OpenSysLog(char *file);
void CloseSysLog(void);

FILE *OpenLogStream(char *file);

#define MAX_LOG_LINE 10240

#endif
