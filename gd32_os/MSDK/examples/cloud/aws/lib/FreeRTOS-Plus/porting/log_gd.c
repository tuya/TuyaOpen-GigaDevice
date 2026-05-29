#include <stdio.h>
#include <stdarg.h>

void vPlatformInitLogging(void)
{

}

/* Log function */
void vLoggingPrintf(const char * pcFormat,
                     ...)
{
    va_list vargs;

    va_start(vargs, pcFormat);
    vprintf(pcFormat, vargs);
    va_end(vargs);
}

