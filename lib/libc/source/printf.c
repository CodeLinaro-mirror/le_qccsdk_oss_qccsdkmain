#include <stdio.h>
#include <stdarg.h>
#include "uart.h"

#define MAX_BUF_LEN 200

int printf(const char* format, ...)
{
    char buf[MAX_BUF_LEN];
    char* ch;
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    ch = buf;
    while( *ch != '\0')
    {
        if(*ch == '\n')
        {
            myputchar('\r');
            myputchar('\n');
        } 
        else
            myputchar((uint8_t)*ch);
        ch++;
    }

    return sizeof(buf);
}
