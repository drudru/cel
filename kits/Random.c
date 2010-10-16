
//
// Random.c
//
// <see corresponding .h file>
// 
//
// $Id: Random.c,v 1.2 2002/02/22 18:45:44 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "svm32.h"
#include "array.h"

#include <stdlib.h>
#include <unistd.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif



//
//  Random - this is the wrapper for the random number routines for Cel
// 

extern char *RandomSL[];

// Yeah, this isn't the best seed, but it works
void Randomseed(void)
{
#ifdef WIN32
    srand(GetTickCount());
#else
    struct timeval tv;
    
    gettimeofday(&tv, NULL);

    srandom(tv.tv_usec + tv.tv_sec);
#endif
    
    VMReturn(Cpu, (unsigned int) nil, 3);
}


// The range is inclusive [bottom top]
void RandomintegerInRange(void)
{
    Proto p;
    Proto arg;
    int top, bottom;
    int i;

    arg    = (Proto) stackAt(Cpu,4);
    arrayGet(arg, 0, &p);
    bottom = objectIntegerValue(p);
    arrayGet(arg, 1, &p);
    top = objectIntegerValue(p);

    // Should do an assertion to verify that 
    // We can handle this integer

    // Ok, here is how it works if the interval is [0 4] we
    // generate a number between [0 1) (notice not inclusive thanks to the
    // addition of a '1' to RAND_MAX. When converting to an int, the
    // C language rounds to floor. So we are generating floating point
    // numbers between [0 5.0) which will give us integers between [0 4]
#ifdef WIN32
    i = (int) ((double) (top - bottom + 1) * rand() / (RAND_MAX+1.0) );
#else
    i = (int) ((double) (top - bottom + 1) * random() / (RAND_MAX+1.0) );
#endif
    i += bottom;
    
    p = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) p, 4);
}


char *RandomSL[] = {
	"seed", "Randomseed", (char *) Randomseed,
	"integerInRange:", "RandomintegerInRange", (char *) RandomintegerInRange,
	""
};

