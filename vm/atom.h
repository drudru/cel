
//
// atom.h
//
// This is the system that handles the unique strings.
// I don't know who came up with calling these "atoms", but
// it is a nice name.
//
// The basic idea is this. Certain strings will need to be unique
// so that checks between these strings can be '==' rather than
// a string compare. This is essential for method comparison in
// Cel objects.
//
// If a string is added, a copy of that string is made, and an integer
// is returned to represent that string. Internally, that integer id is
// used as the key to the actual pointer to the string via a hash. The
// actual string is used in a hashtable as a key to this integer ID.
//
// This allows us to do two things. One, it allows us to determine
// if the string is present by actually using the string as a key.
// Two, it allows us to use a 21 bit integer as a way of getting the
// value of that string.
//
// Since the string literals need to have their lower 11 bits used as
// a routine selector in the proto, we will use a hash table to access
// the strings. This gives us a healthy limit of 2 million strings.
//
// //// For the future we may consider leaving atoms with their owners ??
//
// Why? Well objects will refer to these atoms, and their values are
// hard to change, therefore they can't change if the strings move.
// So for now, we will keep the strings in one spot or we can allocate buffers
// For the strings and keep them in alignment.
// This is what we'll do.
// 
//
// $Id: atom.h,v 1.2 2001/01/05 00:58:54 dru Exp $
//
//
//
//
//


#ifndef H_ATOM
#define H_ATOM

#include "hashtab.h"

typedef char * Atom;

//// This will go away
// only here as a temporary for debugging purposes
//extern strHtab * hashTable;
//extern idHtab  * hashTable;

// Initialize the data structures
void AtomInit(void);

// Derive the string pointer from an atom reference
inline char * atomToString(Atom x);

#define stringToAtom(x)   UniqueString(x)
// Add the string (it makes a copy and returns the pointer to the unique string)
Atom UniqueString(char * str);

#endif
