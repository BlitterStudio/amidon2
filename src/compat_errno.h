#ifndef COMPAT_ERRNO_H
#define COMPAT_ERRNO_H

/* Force inclusion of Roadshow's errno.h */
#include "sys/errno.h"
#ifdef __cplusplus
#include <cstdio>

namespace std { using ::snprintf; }
#endif

/* Define missing POSIX error constants required by C++ stdlib */
#ifndef EBADMSG
#define EBADMSG 82
#endif

#ifndef EIDRM
#define EIDRM 83
#endif

#ifndef EILSEQ
#define EILSEQ 84
#endif

#ifndef ENOLINK
#define ENOLINK 85
#endif

#ifndef ENODATA
#define ENODATA 86
#endif

#ifndef ENOMSG
#define ENOMSG 87
#endif

#ifndef ENOSR
#define ENOSR 88
#endif

#ifndef ENOSTR
#define ENOSTR 89
#endif

#ifndef ENOTSUP
#define ENOTSUP 90
#endif

#ifndef ECANCELED
#define ECANCELED 91
#endif

#ifndef EOWNERDEAD
#define EOWNERDEAD 92
#endif

#ifndef EPROTO
#define EPROTO 93
#endif

#ifndef ENOTRECOVERABLE
#define ENOTRECOVERABLE 94
#endif

#ifndef ETIME
#define ETIME 95
#endif

#ifndef EOVERFLOW
#define EOVERFLOW 96
#endif

#endif // COMPAT_ERRNO_H
