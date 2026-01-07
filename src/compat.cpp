#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern "C" {
    // Stub for strtold (convert to double)
    long double strtold(const char *nptr, char **endptr) {
        return (long double)strtod(nptr, endptr);
    }
    
    // Stub for __xpg_strerror_r
    int __xpg_strerror_r(int errnum, char *buf, size_t buflen) {
        char* s = strerror(errnum);
        size_t len = strlen(s);
        if (len >= buflen) {
             memcpy(buf, s, buflen - 1);
             buf[buflen - 1] = 0;
             return ERANGE;
        }
        strcpy(buf, s);
        return 0;
    }
}
