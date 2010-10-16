
//
// sd2cel.h
//
// This converts a buffer of a module in the SimpleData format to
// active Cel objects
//
//// Note, this needs to have the Globals active in order for this
//// code to be completed - that way I can set the parent's to their
//// proper global
// 
//
// $Id: sd2cel.h,v 1.4 2001/01/05 00:58:54 dru Exp $
//
//
//



#ifndef _CEL_SD2CEL_H
#define _CEL_SD2CEL_H 1



// sd2cel - routines


// Convert the SD memory to real objects

char *  sd2celError(void);

void *  sd2cel(void * mem, unsigned int len);

#endif


