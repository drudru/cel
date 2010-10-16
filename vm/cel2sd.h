
//
// cel2sd.h
//
// This converts a simpler tree of 'simple' objects into Cel objects.
//
// $Id: cel2sd.h,v 1.3 2001/01/05 00:58:54 dru Exp $
//
//



#ifndef _CEL_CEL2SD_H
#define _CEL_CEL2SD_H 1


// C is so lame - I have to have my declarations before my definitions

#include "memstream.h"


// Convert the real objects to SD memory

void *  cel2sd(Proto proto, unsigned int * len);

#endif


