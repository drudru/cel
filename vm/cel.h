
//
// cel.h
//
// This is the low level description of the object structure
// 
//
// $Id: cel.h,v 1.11 2001/11/27 08:35:01 dru Exp $
//
//
//
//


//
// This code is a collection of defines, structures, and 
// macros for working with protos on a low-level
//

#include "alloc.h"

#ifndef H_CEL
#define H_CEL


//typedef unsigned int  uint;
typedef unsigned char uchar;



//
//
//
// PROTO REFERENCES
//
//
//


// These are the types encoded into object references

#define PROTO_REF_PROTO    0x00000000
#define PROTO_REF_INT      0x00000001
#define PROTO_REF_BLOB     0x00000002
#define PROTO_REF_ATOM     0x00000003
#define PROTO_REF_CODE     0x00000004
#define PROTO_REF_CONTEXT  0x00000005
#define PROTO_REF_BLOCK    0x00000006
#define PROTO_REF_UNUSED1  0x00000007
#define PROTO_REF_MASK     (~PROTO_REF_UNUSED1)
#define PROTO_REF_TYPE     (PROTO_REF_UNUSED1)

// Get the type
#define PROTO_REFTYPE(p)      ( ( (uint32) (p) & PROTO_REF_TYPE ) )
// Get the actual ptr
#define PROTO_REFVALUE(p)     ( (Proto) ( (uint32) (p) & PROTO_REF_MASK ) )

// This is an object which is contained in the reference
// oddballs
#define PROTO_REFTYPE_REF(p)    ( (PROTO_REFTYPE(p) & 0x01) )

// This is an object which is contained in the heap
//#define PROTO_REFTYPE_HEAP(p)   ( ((PROTO_REFTYPE(p) & 0x01) == 0) )
#define PROTO_REFTYPE_PROTO(p)  ( (((PROTO_REFTYPE(p) & 0x01) == 0) && (PROTO_REFTYPE(p) != 2)) )

// This indicates if the type of proto referenced would have references to other
// objects inside
#define PROTO_HAS_REFERENCES(p)  ( ((uint32) (p) != 0) && ((PROTO_REFTYPE(p) & 0x01) == 0) )




//
//
//
// PROTO OBJECTS
//
//
//

struct _proto {
    uint32 header;		// Object/Malloc header and size
    uint32 count;		// Number of items in the hash
    uint32 data[1];		// This is actually 1+ entries, don't let the 1 fool you
};

typedef struct _proto * Proto;



    
#define PROTO_SIZEFIELD         0xFFFFFFF0
#define PROTO_GET_SIZE(p)       ( (PROTO_REFVALUE(p))->header & PROTO_SIZEFIELD )

#define PROTO_FLAG_MARK         0x00000001
#define PROTO_GET_MARK(p)       ( (PROTO_REFVALUE(p))->header & PROTO_FLAG_MARK )
#define PROTO_SET_MARK(p)       ( (PROTO_REFVALUE(p))->header |= (PROTO_FLAG_MARK) )
#define PROTO_SET_UNMARK(p)     ( (PROTO_REFVALUE(p))->header &= (~PROTO_FLAG_MARK) )


// This indicates wheter an object is collectable
// or not. If it is, and it isn't marked, then during the
// sweep (if mark/sweep was used), it goes away
// during mark this should be considered an immortal
// (see OPAQUE) 
#define PROTO_FLAG_GC           0x00000002

// Is a proto  / opposite is a blob (this only applies if GC is set)
#define PROTO_FLAG_PROTO        0x00000004
#define PROTO_FLAG_BLOB         0x00000000

#define PROTO_ISPROTO(p)        ( (PROTO_REFVALUE(p))->count & PROTO_FLAG_PROTO )

// This indicates whether a BLOB or PROTO's Data store
// contains object references or just binary data
#define PROTO_FLAG_CLEAR        0x00000008
#define PROTO_ISCLEAR(p)        ( (PROTO_REFVALUE(p))->header & PROTO_FLAG_CLEAR )
#define PROTO_ISOPAQUE(p)       ( !(PROTO_ISCLEAR(p)) )

// See if a proto is a forwarder
#define PROTO_ISFORWARDER(p)     ( (PROTO_REFVALUE(p))->count & PROTO_COUNT_FORWARDER )
#define PROTO_FORWARDVALUE(p)    ( (Proto) ((PROTO_REFVALUE(p))->data[0]) )

// Proto Size Field 

#define PROTO_COUNT_DATAMASK     (0xFF)
#define PROTO_COUNT_SLOTMASK     (0xFF00)
// deprecated because we have the new reference types
#define PROTO_COUNT_TYPEISCODE   0x80000000
// This indicates that an object has moved, the reference should
// be updated, this is only relevant to Proto's, not blobs
#define PROTO_COUNT_FORWARDER    0x40000000


#define protoDataCount(p)    (((PROTO_REFVALUE(p))->count & PROTO_COUNT_DATAMASK) >> 0)
#define protoSlotCount(p)    (((PROTO_REFVALUE(p))->count & PROTO_COUNT_SLOTMASK) >> 8)

// These don't normalize to a proto (from a CODE or BLOCK reference)
// So far, this is ok since all the routines that use them do a check.
#define protoSetDataCount(p,i)   ((p)->count = ((p)->count & ~PROTO_COUNT_DATAMASK) | ( (i) << 0) )
#define protoSetSlotCount(p,i)   ((p)->count = ((p)->count & ~PROTO_COUNT_SLOTMASK) | ( (i) << 8) )

#define protoSlotData(p,s)      ((Proto) (PROTO_REFVALUE(p))->data[slotIndex((s))] )
#define protoDataAtIndex(p,s)   ((Proto) (PROTO_REFVALUE(p))->data[(s)] )


//
//
//
// SLOT DATA
//
//
//

// These are fields related to sections of an allocated proto
// on the heap.

struct _protoEntry 
{
    uint32 keyActionIndex;	// Lower 8 bits are the index, the next 3 bits indicate the action, the upper 21 are the key (an atom)
};

typedef struct _protoEntry * ProtoSlot;


// This is the special case of the proto's key in a slot
// Since we use the lower 3 bits for the action code, the
// Key must have these masked out. So, when atom pointers
// are returned by that module, they are acutally left shifted
// by 1 
#define PROTO_SLOT_KEYMASK     (0xFFFFF800)
#define PROTO_SLOT_ACTIONMASK  (0x700)
#define PROTO_SLOT_KEYACTIONMASK     (PROTO_SLOT_KEYMASK | PROTO_SLOT_ACTIONMASK)
#define PROTO_SLOT_ACTIONMASK  (0x700)
#define PROTO_SLOT_INDEXMASK   (0xFF)

// Notice that we don't shift down the key, this is because it's normal form
// is left shifted 11 binary digits
#define slotKey(p)    ( ((p)->keyActionIndex & PROTO_SLOT_KEYMASK) | PROTO_REF_ATOM  ) 
#define slotAction(p) (((p)->keyActionIndex & PROTO_SLOT_ACTIONMASK) >> 8  )
#define slotIndex(p)  (((p)->keyActionIndex & PROTO_SLOT_INDEXMASK)  >> 0  )

// Again, notice that we don't shift down the key, this is because it's normal form
// is left shifted 11 binary digits, plus we remove the PROTO_REF_MASK bits
// with the argument before it is OR'd in
#define slotSetKey(p,k)     ((p)->keyActionIndex = ((p)->keyActionIndex & ~PROTO_SLOT_KEYMASK)    |  ((k) & PROTO_SLOT_KEYMASK) )
#define slotSetAction(p,a)  ((p)->keyActionIndex = ((p)->keyActionIndex & ~PROTO_SLOT_ACTIONMASK) | ((a) << 8)  )
#define slotSetIndex(p,i)   ((p)->keyActionIndex = ((p)->keyActionIndex & ~PROTO_SLOT_INDEXMASK)  |  (i)        )



#endif

