
//
// atom.c
//
// <see corresponding .h file>
// 
//
// $Id: atom.c,v 1.6 2002/02/22 22:23:24 dru Exp $
//
//
//
//
//

#include <unistd.h>
#include <string.h>


#include "atom.h"
#include "hashtab.h"
#include "cel.h"


int pageSize = 4096;
int memLeftInPage;
char * stringPtr;


htab  * strHashTable;
htab  * idHashTable;

int     stringIdCount;


char * makeCopyOfString(char * str);



void AtomInit(void)
{
    memLeftInPage = 0;
    stringPtr = NULL;
    
    strHashTable  = celhcreate(8);
    idHashTable   = celhcreate(8);
    stringIdCount = 0;

    UniqueString("BublBop");
}


// This derives the string pointer out of an Atom reference
inline char * atomToString(Atom x)
{
    int i;
    int j;

    j = (uint32) x >> 11;	// Shift the reference to the right to get the id.
    
    i = hfindInt(idHashTable, j);

    if (i) {
	return hstuff(idHashTable);
    } else {
	return (char *)0;
    }
    
}



// Add the string (it makes a copy and returns the pointer to the unique string)
Atom UniqueString(char * str)
{
    int i;
    void * val;

    // check if string is in hash table (hash table hashes string and searches in table)
    i = hfind(strHashTable, str, strlen(str));

    if (i != TRUE) {
	// If it isn't make a copy, add it to table,
	// We must do this so we keep the strings and we don't have
	// to worry about them going away (the hash routines just store
	// pointers to keys and data)
	val = (char *) makeCopyOfString(str);
	
	// Add the string as the data for the integer->string hashtable
	haddInt(idHashTable, stringIdCount,    val);
	
	// Add the integer as the data for the string->integer hashtable
	hadd(strHashTable,   val, strlen(val), (char *)stringIdCount);
	val = (void *) stringIdCount;
	stringIdCount++;
	
    } else {
	val = hstuff(strHashTable);
    }

    // Make the returned pointer of string literal type
    (unsigned int) val <<= 11;
    (unsigned int) val |=  PROTO_REF_ATOM;
	
    return (Atom) val;
}


// This routine add strings to a page sized buffer. If the buffer
// overflows, it creates a new buffer. It also insures that the strings
// get inserted on a 4 byte alignment
//// For memory freeing purposes, we should store a list of the pages used for this
char * makeCopyOfString(char * str)
{
    int len;

    len = strlen(str);
    len++;			// Include space for the zero
    

    //Check the space left in the page, if there isn't any
    //space left, allocate a new one
    if (len > memLeftInPage) {
	memLeftInPage = pageSize;
	stringPtr = (char *) celmalloc(pageSize); // I don't have to worry about alignment's of malloc's they
	                              // are always 16 bytes which is within our 4 byte alignment
    }

    
    strcpy(stringPtr, str);	// Copy the string to the buffer
    str = stringPtr;		// Preserve the pointer to this string
    
    stringPtr += len;
    memLeftInPage -= len;

    return str;
}
