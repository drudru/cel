
//
// array.c
//
// <see corresponding .h file>
// 
//
// $Id: array.c,v 1.15 2002/01/28 23:15:22 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"

// Globals


//
// Array codes
//

Proto createArray(Proto title)
{
    Proto tmp;
    Proto blob;

    GCOFF();
    
    tmp = objectNew(0);
    objectSetSlot(tmp, ATOMname, PROTO_ACTION_GET, (uint32) title);
    objectSetSlot(tmp, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(0));
    objectSetSlot(tmp, ATOM_capacity, PROTO_ACTION_GET, (uint32) objectNewInteger(8));

    blob = objectNewObjectBlob(8);
    objectSetSlot(tmp, ATOM_array, PROTO_ACTION_GET, (uint32) blob);
    objectSetSlot(tmp, ATOMparent, PROTO_ACTION_GET, (uint32) ArrayParent);

    GCON();
    
    return tmp;
}

Proto createArraySize(Proto title, int size)
{
    Proto tmp;
    Proto blob;

    GCOFF();

    tmp = objectNew(0);
    objectSetSlot(tmp, ATOMname, PROTO_ACTION_GET, (uint32) title);
    objectSetSlot(tmp, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(0));
    objectSetSlot(tmp, ATOM_capacity, PROTO_ACTION_GET, (uint32) objectNewInteger(size));

    blob = objectNewObjectBlob(size);
    objectSetSlot(tmp, ATOM_array, PROTO_ACTION_GET, (uint32) blob);
    objectSetSlot(tmp, ATOMparent, PROTO_ACTION_GET, (uint32) ArrayParent);
    
    GCON();

    return tmp;
}


Proto addToArray (Proto array, Proto newItem)
{
    Proto tmp;
    int i;
    int count;
    int capacity;
    
    
    i = objectGetSlot(array, ATOMsize, &tmp);

    // A zero count means there is one

    assert(i);			// It can only add it if this is a array
    count = objectIntegerValue(tmp);
    count++;

    i = objectGetSlot(array, ATOM_capacity, &tmp);
    assert(i);			// It can only add it if this is a array
    capacity = objectIntegerValue(tmp);

    i = objectGetSlot(array, ATOM_array, &tmp);
    assert(i);			// It can only add it if this is a array

    if (capacity == count) {
	// Time to resize
	tmp = objectBlobResize(tmp, capacity << 1);
	objectSetSlot(array, ATOM_array, PROTO_ACTION_GET, (uint32) tmp);
	objectSetSlot(array, ATOM_capacity, PROTO_ACTION_GET, (uint32) objectNewInteger((capacity << 1)));
    }

    objectBlobAtSet(tmp, (count - 1), newItem);
    objectSetSlot(array, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(count));
	
    return array;
}



Proto annotateArray(Proto array, Atom name, Proto newItem)
{
    Proto tmp;
    int i;
    
    i = objectGetSlot(array, stringToAtom("_annotate"), &tmp);

    // If it isn't there, add it
    if (0 == i) {
	GCOFF();		// Just to be on the safe side
	
	tmp = objectNew(0);
	objectSetSlot(array, stringToAtom("_annotate"), PROTO_ACTION_GET, (uint32) tmp);
	
	GCON();
    }
    
    objectSetSlot(tmp, name, PROTO_ACTION_GET, (uint32) newItem);
	
    return array;
}



Proto removeFromArray(Proto array, int index)
{
    Proto tmp;
    Proto blob;
    int total, i;
    

    // Ok, loop from index +1 to end
    // Set all of the slots lower

    i = objectGetSlot(array, ATOMsize, &tmp);
    assert(i);			// It can only add it if this is a array
    
    total = objectIntegerValue(tmp);

    i = objectGetSlot(array, ATOM_array, &blob);
    assert(i);			// It can only add it if this is a array
    
    i = index;
    
    assert( (i >= 0) && (i < total));
    
    while (1) {

	if (i >= (total - 1)) {
	    break;
	}
	
	tmp = objectBlobAtGet(blob, (i+1));
	objectBlobAtSet(blob, i, tmp);
	i++;
    }

    // Clear the entry
    objectBlobAtSet(blob, i, nil);
    
    // Decrease the total count
    total--;
    objectSetSlot(array, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(total));

    return array;
}

Proto arrayInsertObjAt(Proto destarray, Proto obj, int index)
{
    Proto tmp;
    Proto blob;
    int total, i;

    // Ok, loop from index +1 to end
    // Set all of the slots lower

    i = objectGetSlot(destarray, ATOMsize, &tmp);
    assert(i);			// It can only work if this is a array
    
    i = objectGetSlot(destarray, ATOM_array, &blob);
    assert(i);			// It can only work if this is a array

    total = objectIntegerValue(tmp);
    i = index;

    // Catch the invalid case
    if ((i < 0) || (i > (total - 1))) {
	assert(0);
    }
    
    // Handle the simple case
    if ((total) == i) {
	// Create the space
	addToArray(destarray, obj);
	return destarray;
    } else {
	// Create the space
	addToArray(destarray, nil);
    }

    i = total - 1;
    
    while (1) {

	if (i < index) {
	    break;
	}
	
	tmp = objectBlobAtGet(blob, i);
	objectBlobAtSet(blob, (i+1), tmp);
	i--;
    }

    // Finally, place the new obj, buff1 should still be set right
    objectBlobAtSet(blob, (i+1), obj);
    
    return destarray;
}


Proto appendArrayToArray(Proto src, Proto dest)
{
    Proto tmp;
    Proto blob;
    int i,j;
    
    
    i = objectGetSlot(src, ATOMsize, &tmp);
    assert(i);

    i = objectGetSlot(src, ATOM_array, &blob);
    assert(i);

    
    i = objectIntegerValue(tmp);

    for (j = 0; j < i; j++ ) {
	tmp = objectBlobAtGet(blob, j);
	addToArray(dest, tmp);
    }

    return dest;
}

uint32 arrayCount(Proto obj)
{
    Proto tmp;
    uint32 count;
    int i;
    
    i = objectGetSlot(obj, ATOMsize, &tmp);
    assert(i);

    count = objectIntegerValue(tmp);
    return count;
}

uint32 arrayGet (Proto obj, uint32 index, Proto * dest)
{
    Proto tmp;
    Proto blob;
    uint32 count;
    int i;
    
    i = objectGetSlot(obj, ATOMsize, &tmp);
    assert(i);

    count = objectIntegerValue(tmp);
    assert(count > index);
    
    i = objectGetSlot(obj, ATOM_array, &blob);
    assert(i);


    *dest = objectBlobAtGet(blob, index);
    
    return 1;
}

uint32 arraySet (Proto obj, uint32 index, Proto set)
{
    Proto tmp;
    Proto blob;
    uint32 count;
    int i;
    
    i = objectGetSlot(obj, ATOMsize, &tmp);
    assert(i);

    count = objectIntegerValue(tmp);
    assert(count > index);
    
    i = objectGetSlot(obj, ATOM_array, &blob);
    assert(i);


    objectBlobAtSet(blob, index, set);
    
    return 1;
}


int isArray(Proto proto)
{
    Proto tmp;
    
    return (objectGetSlot(proto, ATOM_array, &tmp));
}

// Remove forwards
void * removeForwards (Proto proto)
{
    int       i, j, k;
    Proto     newObj;
    Proto     blob;
    Proto     tmp;
    ProtoSlot slotPtr;

    
    // Determine the type of reference this is
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	return proto;
    }
    
    switch (i) {
	case PROTO_REF_PROTO:
	case PROTO_REF_BLOCK:
	case PROTO_REF_CODE:
	    // If this is a forward, flatten it
	    while (PROTO_ISFORWARDER(proto)) {
		proto = PROTO_FORWARDVALUE(proto);
	    }

	    // enumerate through the proto
	    j = 0;
	    while (1) {
		slotPtr = objectEnumerate(proto, &j, 1);
		if (slotPtr == nil) {
		    break;
		}
		k = slotAction(slotPtr);
		switch (k) {
		    case PROTO_ACTION_GET:
		    case PROTO_ACTION_SET:
		    case PROTO_ACTION_PASSTHRU:
			// Access the data and print it
			newObj = removeForwards(protoSlotData(proto, slotPtr));
			objectSetSlot(proto, (Atom) slotKey(slotPtr), k, (uint32) newObj);
			break;
		}
	    }

	    
	    i = objectGetSlot(proto, ATOMsize, &tmp);
	    if (i) {
		i = objectGetSlot(proto, ATOM_array, &blob);
		assert(i);
		j = objectIntegerValue(tmp);
		
		for (i = 0; i <= j; i++) {
		    tmp = objectBlobAtGet(blob, i);
		    newObj = removeForwards(tmp);
		    objectBlobAtSet(blob, i, newObj);
		}
	    }
	    
	    break;
    }
    return proto;
}
