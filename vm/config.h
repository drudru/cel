
// Config.h


#ifdef WIN32

#include <sys/utime.h>

#endif

#ifdef __MACH__
#define unix
#define unixbsd
#endif

#ifdef unix

#include <utime.h>
#include <errno.h>
#include <sys/types.h>

#endif

