
//
// array.h
//
// This library handles protos that are arrays.
// They are used internally by the parser and code generator
//
//
// $Id: array.h,v 1.8 2001/03/03 09:01:53 dru Exp $
//
//
//



#ifndef _CEL_ARRAY_H
#define _CEL_ARRAY_H 1


Proto createArray(Proto title);

Proto createArraySize(Proto title, int size);

Proto addToArray(Proto array, Proto newItem);

Proto annotateArray(Proto array, Atom name, Proto newItem);

Proto removeFromArray(Proto array, int index);

Proto arrayInsertObjAt(Proto destarray, Proto obj, int index);

uint32 arrayCount(Proto obj);

uint32 arrayGet (Proto obj, uint32 index, Proto * dest);

uint32 arraySet (Proto obj, uint32 index, Proto set);

int isArray(Proto proto);

Proto appendArrayToArray(Proto src, Proto dest);

void * removeForwards(Proto proto);



#endif


