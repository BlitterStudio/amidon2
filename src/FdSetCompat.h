/*
 * Amidon2 - a Mastodon client for AmigaOS
 * Copyright (C) 2026 Dimitris Panokostas
 *
 * Compatible fd_set definition for bsdsocket.library WaitSelect.
 *
 * The clib2 <sys/socket.h> does not define fd_set/FD_ZERO/FD_SET,
 * and the NDK version is shadowed by the clib2 include path.
 * We define our own layout-compatible type instead.
 *
 * bsdsocket.library expects fd_set as an array of 8 unsigned longs
 * (256 bits on 32-bit m68k), which is what this struct provides.
 * WaitSelect() casts fd_set* to APTR, so layout compatibility is
 * the only requirement.
 */
#ifndef FDSET_COMPAT_H
#define FDSET_COMPAT_H

#include <cstring>

extern "C" {
#include <exec/types.h>
}

#define AMIDON_FD_SETSIZE 256
#define AMIDON_NBBY 8
#define AMIDON_NFDBITS (sizeof(ULONG) * AMIDON_NBBY)
#define AMIDON_HOWMANY(x, y) (((x) + ((y) - 1)) / (y))

typedef struct amidon_fd_set {
    ULONG fds_bits[AMIDON_HOWMANY(AMIDON_FD_SETSIZE, AMIDON_NFDBITS)];
} amidon_fd_set;

#define AMIDON_FD_ZERO(p) ((void)memset((p), 0, sizeof(*(p))))
#define AMIDON_FD_SET(n, p) \
    ((void)(((ULONG)(n) < AMIDON_FD_SETSIZE) ? \
     ((p)->fds_bits[(ULONG)(n) / AMIDON_NFDBITS] |= (1UL << ((ULONG)(n) % AMIDON_NFDBITS))) : 0))
#define AMIDON_FD_CLR(n, p) \
    ((void)(((ULONG)(n) < AMIDON_FD_SETSIZE) ? \
     ((p)->fds_bits[(ULONG)(n) / AMIDON_NFDBITS] &= ~(1UL << ((ULONG)(n) % AMIDON_NFDBITS))) : 0))
#define AMIDON_FD_ISSET(n, p) \
    (((ULONG)(n) < AMIDON_FD_SETSIZE) && \
     ((p)->fds_bits[(ULONG)(n) / AMIDON_NFDBITS] & (1UL << ((ULONG)(n) % AMIDON_NFDBITS))))

#endif /* FDSET_COMPAT_H */