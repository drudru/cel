
//
// cel2sd.c
//
// <see corresponding .h file>
// 
//
// $Id: cel2sd.c,v 1.12 2002/02/22 22:23:24 dru Exp $
//
//


#include "crc.h"
#include "cel.h"
#include "runtime.h"
#include "memstream.h"
#include "cel2sd.h"


static void * realMapObjects(Proto proto, htab * ht);
static void * realCel2sd(Proto proto, MemStream ms, htab * ht);
static int    writeSDString(MemStream ms, Proto proto, int32 id); 
static void   assignID(Proto proto, htab * ht);
static int    getNewID(htab * ht);


//
// Brief intro:
// The objects get mapped to a hash table for references (realMapObjects)
// Then the objects get serialized with those ID's (realCel2sd)
//


// Convert the real objects to SD memory
// 
void *  cel2sd(Proto proto, unsigned int * length)
{
    MemStream ms;
    uint32    ck;
    char    * str = "AqUa SimpleData\n002\nc  \n";
    htab    * ht;
    char    * result;
    
    
    // Setup a memory stream to work over the memory
    ms = NewMemStream((void *) 0, 0);

    // Write header
    writeMSCSizeFrom(ms, strlen(str), str);
    writeMSCInt(ms, 0);		// Length of module - we'll go back and rewrite this

    // Generate hash table of all objects, generate an object->integer mapping
    ht = celhcreate(8); 
    // Address zero will hold the last used number - we'll start at 32
    haddInt(ht, 0, (void *)32);
    realMapObjects(proto, ht);

    realCel2sd(proto, ms, ht);
    
    
    // Write CRC
    *length = getMSSize(ms);
    seekMS(ms, 24);
    writeMSCInt(ms, *length);
    
    seekMS(ms, *length);
    ck = celcrc32(ms->start, *length);
    writeMSCInt(ms, ck);
    
    *length = getMSSize(ms);
    result  = ms->start;
    
    freeMemStream(ms);
    
    return result;
}

// This routine will map objects onto a hash table to prepare for
// object serialization
void * realMapObjects(Proto proto, htab * ht)
{
    int i,j;
    struct _protoEntry * slotPtr;
    Proto p1;
    int size;
    Proto tmp;

    // Determine the type of reference this is
    i = (uint32) proto & PROTO_REF_TYPE;
    p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);

    if (proto == nil) {
	return proto;
    }
    

    switch (i) {
	case PROTO_REF_INT:
	    return nil;
	    break;
	case PROTO_REF_PROTO:
	    // If this is a forward
	    if (PROTO_ISFORWARDER(p1)) {
		realMapObjects(PROTO_FORWARDVALUE(p1), ht);
	    } else {
	        // Assign the dictionary an id
	        assignID(proto, ht);

		//// WHY IS THIS DONE? 1/13/01
		i = objectSlotCount(proto);
		
		// enumerate through the proto showing it's contents
		// *Note* we iterate through the slots, not the data
		// section. (Some slots may point to the same data slot)
		j = 0;
		while (1) {
		    // Note: Skip the parent slot
		    slotPtr = objectEnumerate(proto, &j, 1);
		    if (slotPtr == nil) {
			break;
		    }

                    //// Note, this ignores dictionaries that have slots other than 'get'
		    //// type actions
		    
		    // Map the key
		    realMapObjects((Proto) slotKey(slotPtr), ht);

		    // Map the data
		    realMapObjects(protoSlotData(proto, slotPtr), ht);
		}
	    }
	    
	    break;
	case PROTO_REF_BLOB:
	    // Assign the blob an id
	    assignID(proto, ht);
	    
	    // Need to see if this is a GC'able blob
	    if ( PROTO_ISCLEAR(proto) ) {
		size = objectBlobSize(proto);
		size = size >> 2;		// Each 4 bytes represents an object
		
		// enumerate through the blob
		for (i = 0; i < size; i++) {
		    tmp = objectBlobAtGet(proto, i);
		    realMapObjects(tmp, ht);
		}
	    }
	    break;
	case PROTO_REF_ATOM:

	    // Assign the dictionary an id
	    assignID(proto, ht);
	    
	    return proto;
	    break;
    }

    return proto;
}

void assignID(Proto proto, htab * ht)
{
  int i;

  // If it isn't already there, generate a new number
  if ( hfindInt(ht, (ub4) proto) ) {
    // Do nothing
  } else {
    i = getNewID(ht);
    haddInt(ht, (ub4) proto, (void *)i);
  }
}

int getNewID(htab * ht)
{
  int i;

  // Get the counter - no object should be '0'
  if ( hfindInt(ht, 0) ) {
      i = (uint32) hstuff(ht);
      i++;
      // Store it back
      hstuff(ht) = (void *) i;
  } else {
      i = 32;
      haddInt(ht, 0, (void *) i); 
  }
  
  return i;
}


// The routine that does the work on the dumping of objects to the SimpleData format
void * realCel2sd(Proto proto, MemStream ms, htab * ht)
{
    int32 i;
    int   j;
    int   k;
    Proto p1;
    Proto p;
    int   hasParent;
    int   parentIndex = -1;
    struct _protoEntry * slotPtr;
    uint32 size;
    char * cp;
    
    int      dataCount = 0;
    uint32 * transTable;
    uint32 * valueTable;
    
    

    // Determine the type of reference this is
    i = (uint32) proto & PROTO_REF_TYPE;
    p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);

    if (proto == nil) {
	writeMSCChar(ms, 'N');
	return proto;
    }
    if (proto == TrueObj) {
	writeMSCChar(ms, 't');
	return proto;
    }
    if (proto == FalseObj) {
	writeMSCChar(ms, 'f');
	return proto;
    }


    switch (i) {
	case PROTO_REF_INT:
	    writeMSCChar(ms, 'I');
	    writeMSCInt(ms, objectIntegerValue(proto) );
	    break;
	case PROTO_REF_PROTO:
	    // If this is a forward
	    if (PROTO_ISFORWARDER(p1)) {
		realCel2sd(PROTO_FORWARDVALUE(p1), ms, ht);
	    } else {
		
	        assert ( hfindInt(ht, (ub4) proto) );
  	        i = (int32) hstuff(ht);
	        if (i > 0) {
	          // Store the number as a negative value
	          (int32) hstuff(ht) = -i;

     	  	  writeMSCChar(ms, '{'); // Start of dictionary
	          // Write the reference ID
	          writeMSCInt(ms, (int32) i);


		  // All of this
		  // to skip the parent
		  i = objectLookupKey(proto, (uint32) ATOMparent, &slotPtr);
		  if (i) {
		      hasParent = 1;
		      k = slotAction(slotPtr);
		      if (k != PROTO_ACTION_GET) {
			  assert(0);
		      }
		      parentIndex = slotIndex(slotPtr);
		  } else {
		      hasParent = 0;
		  }
		  

		  // Build translation table
		  i = objectDataCount(proto);
		  dataCount = 0;
		  transTable = (uint32 *) alloca(sizeof(uint32) * i);
		  valueTable = (uint32 *) alloca(sizeof(uint32) * i);
		  memset((void *) valueTable, 0xff, (sizeof(uint32) * i));
		  
		  // enumerate through the proto marking the real data
		  // section. (Some slots may point to the same data slot)
		  // and some data slots may be garbage
		  j = 0;
		  while (1) {
		    // Note: it skips the parent slots
		    slotPtr = objectEnumerate(proto, &j, 1);
		    if (slotPtr == nil) {
			break;
		    }
		    if ( (valueTable[slotIndex(slotPtr)]) == 0xFFFFFFFF) {
			transTable[dataCount] = slotIndex(slotPtr);
			valueTable[slotIndex(slotPtr)] = dataCount;
			dataCount++;
		    }
		  }
		  

		      
		  // Write the data count
	    	  writeMSCChar(ms, 'I');
		  writeMSCInt(ms, dataCount);

		  // Write the slot count
		  j = objectSlotCount(proto);
	    	  writeMSCChar(ms, 'I');
		  if (hasParent) {
		      j--;
		  }
		  writeMSCInt(ms, (j));


		  


		  // enumerate through the data section of proto
		  for (j = 0; j < dataCount; j++) {
		      realCel2sd(protoDataAtIndex(proto, transTable[j]), ms, ht);
		  }

		  // enumerate through the proto showing it's contents
		  // *Note* we iterate through the slots, not the data
		  // section. (Some slots may point to the same data slot)
		  j = 0;
		  while (1) {
		    // Note: it skips the parent slots
		    slotPtr = objectEnumerate(proto, &j, 1);
		    if (slotPtr == nil) {
			break;
		    }

	            // Write the key
		    realCel2sd((Proto) slotKey(slotPtr), ms, ht);
		    writeMSCChar(ms, slotAction(slotPtr));
		    writeMSCChar(ms, valueTable[slotIndex(slotPtr)]);

		  }
		  writeMSCChar(ms, '}' ); // End of dictionary
		} else {
	          // Write the reference
	          writeMSCChar(ms, 'r');
	          writeMSCInt(ms, (int32) -i);
		}
	    }
	    
	    break;
	case PROTO_REF_ATOM:
	    assert ( hfindInt(ht, (ub4) proto) );
	    i = (int32) hstuff(ht);
	    if (i > 0) {
	      // Store the number as a negative value
	      (int32) hstuff(ht) = -i;
	      // Write the reference
	      writeSDString(ms, proto, i);
	    } else {
	      // Write the reference
	      writeMSCChar(ms, 'r');
	      // The integer is already negative, therfore negating it should be
	      // the proper id
	      writeMSCInt(ms, (int32) -i);
	    }
	    break;
	case PROTO_REF_BLOB:
	    // If the blob is internally GC'able, then
	    // Write out as array, else write out as binary
	    // data

	    if (PROTO_ISCLEAR(proto)) {
		assert ( hfindInt(ht, (ub4) proto) );
		i = (int32) hstuff(ht);
		if (i > 0) {
		    // Store the number as a negative value
		    (int32) hstuff(ht) = -i;
		    // Write the reference
		    
		    size = objectBlobSize(proto);
		    size = size >> 2;
	      
		    // Write string
		    writeMSCChar(ms, '(');
		    // Write reference
		    writeMSCInt(ms, i);
		    writeMSCInt(ms, size);
		    for (i = 0; i < size; i++) {
			p = objectBlobAtGet(proto, i);
			realCel2sd(p, ms, ht);
		    }
		    writeMSCChar(ms, ')');
		} else {
		    // Write the reference
		    writeMSCChar(ms, 'r');
		    // The integer is already negative, therfore negating it should be
		    // the proper id
		    writeMSCInt(ms, (int32) -i);
		}
	    } else {
		//
		// Write the OPAQUE data
		// 
		assert ( hfindInt(ht, (ub4) proto) );
		i = (int32) hstuff(ht);
		if (i > 0) {
		    // Store the number as a negative value
		    (int32) hstuff(ht) = -i;
		    // Write the reference
		    
		    size = objectBlobSize(proto);
	      
		    // Write string
		    writeMSCChar(ms, 'B');
		    // Write reference
		    writeMSCInt(ms, i);
		    writeMSCInt(ms, size);
		    cp = objectPointerValue(proto);
		    writeMSCSizeFrom(ms, size,  cp);
		    writeMSCChar(ms, 'b');
		} else {
		    // Write the reference
		    writeMSCChar(ms, 'r');
		    // The integer is already negative, therfore negating it should be
		    // the proper id
		    writeMSCInt(ms, (int32) -i);
		}
	    }
	    
	    break;
        default:
            assert(0);
    }

    return proto;
    
}


int writeSDString(MemStream ms, Proto proto, int32 id) 
{
    char * p;
    int i;
    
    p = atomToString((Atom) proto);
    i = strlen(p);
    i |= 0x80;
    // Write string
    writeMSCChar(ms, (unsigned char) i);
    // Write reference
    writeMSCInt(ms, id);
    writeMSCSizeFrom(ms, (i & 0x7f), p);
    writeMSCChar(ms, '~');
    return 0;
}

