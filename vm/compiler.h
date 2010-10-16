
//
// compiler.h
//
// This converts strings specified in a buffer into a parse
// tree of Aqua code and Aqua objects
//
//
//// Note, this needs to have the Globals active in order for this
//// code to be completed - that way I can set the parent's to their
//// proper global
// 
//
// $Id: compiler.h,v 1.7 2001/01/05 00:58:54 dru Exp $
//
//
//
//



#ifndef _CEL_COMPILE_H
#define _CEL_COMPILE_H 1


// C is so lame - I have to have my declarations before my definitions

#include "memstream.h"

// The token

struct CompilerTokenStruct
{
    MemStream file;
    int start;             // Position in the file
    int end;
    int token;
    int intValue;
    int stringValue;
    char * binaryValue;
};


typedef struct CompilerTokenStruct * CompilerToken;



// sd2cel - routines


// Convert the SD memory to real objects

char *  compileError(void);

void compileSetError(char * str);

Proto   eval(char * p, int parseShow, int codeGenShow);

void *  compile(MemStream memStream, int parseShow, int codeGenShow);


#endif


