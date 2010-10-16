
//
// Math.c
//
// <see corresponding .h file>
// 
//
// $Id: Math.c,v 1.2 2002/02/22 18:45:44 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "svm32.h"
#include "array.h"

#include <math.h>

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

//
//  Math - this is the wrapper for the random number routines for Cel
// 

extern char *MathSL[];

// Return the log (base10) of the argument
void Mathlog(void)
{
    Proto p;
    Proto arg;
    double f;
    

    arg    = (Proto) stackAt(Cpu,4);

    f = objectFloatValue(arg);
    
    f = log10(f);
    
    p = objectNewFloat(f);

    VMReturn(Cpu, (unsigned int) p, 4);
}

// Return the result of the number raised to a power
void MathnumberraisedTo(void)
{
    Proto p;
    Proto arg1;
    Proto arg2;
    double base, power;
    double f;
    

    arg1    = (Proto) stackAt(Cpu,4);
    arg2    = (Proto) stackAt(Cpu,5);

    base  = objectFloatValue(arg1);
    power = objectFloatValue(arg2);
    
    f = pow(base, power);
    
    p = objectNewFloat(f);

    VMReturn(Cpu, (unsigned int) p, 5);
}

// Return the sine of the argument
// The argument is in radians
void Mathsine(void)
{
    Proto p;
    Proto arg;
    double f;
    

    arg    = (Proto) stackAt(Cpu,4);

    f = objectFloatValue(arg);
    
    f = sin(f);
    
    p = objectNewFloat(f);

    VMReturn(Cpu, (unsigned int) p, 4);
}

// Return pi
void Mathpi(void)
{
    Proto p;

    p = objectNewFloat(M_PI);

    VMReturn(Cpu, (unsigned int) p, 3);
}

// Return the conversion of an angle in
// degress to radians
void MathdegreesToRadians(void)
{
    Proto p;
    Proto arg;
    double f;
    

    arg    = (Proto) stackAt(Cpu,4);

    f = objectFloatValue(arg);
    
    f = f / 180.0 * M_PI;
    
    p = objectNewFloat(f);

    VMReturn(Cpu, (unsigned int) p, 4);
}

char *MathSL[] = {
	"log:", "Mathlog", (char *) Mathlog,
	"number:raisedTo:", "MathnumberraisedTo", (char *) MathnumberraisedTo,
	"sine:", "Mathsine", (char *) Mathsine,
	"pi", "Mathpi", (char *) Mathpi,
	"degreesToRadians:", "MathdegreesToRadians", (char *) MathdegreesToRadians,
	""
};

