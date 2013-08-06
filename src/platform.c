#include "platform.h"


#if defined(_WIN32) || defined(_WIN64)
#include <math.h>

double round(float v)
{
    return floor(v + 0.5f);
}

#endif


#if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64) 

char * strndup(const char *s, size_t n)
{
    char *result = malloc(n + 1);
    if (result == NULL)
        return NULL;
    
    memcpy(result, s, n);
    result[n] = 0;
    return result;
}

#endif
