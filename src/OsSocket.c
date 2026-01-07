#define __USE_INLINE__
#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <inline/bsdsocket.h>
// #include <bsdsocket/socket.h> // Missing in this env

// External refs
extern struct Library *SocketBase;
extern int printf(const char *, ...); // Use clib2's printf for debugging

// Manual prototype for AmigaOS Socket function (since headers are failing us)
long Socket(long domain, long type, long protocol);

long CreateRawAmigaSocket(long domain, long type, long protocol) {
    if (!SocketBase) {
        printf(">>> OsSocket ERROR: SocketBase is NULL!\n");
        return -1;
    }
    
    // printf(">>> OsSocket: Calling Socket(%ld, %ld, %ld) with SocketBase=%p\n", domain, type, protocol, SocketBase);
    
    // Call AmigaOS Socket()
    long sock = Socket(domain, type, protocol);
    
    if (sock < 0) { // Check for failure (usually -1)
         // long err = Errno();
         printf(">>> OsSocket: Socket() failed. Res: %ld\n", sock);
         return -1;
    }
    
    // AmigaOS socket handles are pointers (0x...) so usually large positive integers.
    if (sock == 0) {
         printf(">>> OsSocket WARNING: Socket() returned 0? (NULL)\n");
    }
    
    return sock;
}
