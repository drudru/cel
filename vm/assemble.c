
//
// assemble.c
//
// <see corresponding .h file>
// 
//
// $Id: assemble.c,v 1.18 2001/11/27 08:35:01 dru Exp $
//
//
//
//

#include "alloc.h"
#include "runtime.h"
#include "atom.h"
#include "celendian.h"
#include "assemble.h"
#include "array.h"



char errMessage[80];


char *  assembleError(void)
{
    return errMessage;
}


void setError(char * str)
{
    strcpy(errMessage, str);
}



// Convert the assembly source into code

// This function takes a prot. It looks up the given slot
// The slot should have an '_assemble' key. If it does, 
// the routine generates a codeproto for the code.
// It then overwrites the original slot in the proto with
// a passthru to this codeproto.
// Example: Root->_Start->_assemble
// You would call assemble on (Root, "_Start")

Proto assemble (Proto proto, Atom slot, int isBlock)
{
    int        i;
    int        size;
    uint32     j;
    int        offset;
    int        k;
    Proto      tmpObj;
    Proto      codeProto;
    Proto      codeBlob;
    Proto      source;
    Proto      assembly;
    Proto      opCode;
    Proto      opType;
    Proto      operand;
    Proto      tmp2;
    uchar *    p;
    int        count;
    
    

    if (isBlock) {
	codeProto = proto;
	// Set the parent, but note that the parent really gets set
	// during the push
	objectSetSlot(codeProto, ATOMparent, PROTO_ACTION_GET, (uint32) nil );
	objectSetSlot(codeProto, ATOMblockParent, PROTO_ACTION_GET, (uint32) nil );
    } else {
	// Get the proto that is the codeProto
	objectGetSlot(proto, slot, &codeProto);
	
	// Set the parent
	objectSetSlot(codeProto, ATOMparent, PROTO_ACTION_GETFRAME, (uint32) objectNewInteger(3) );
    }

    // The source is in the slot "Assemble"
    objectGetSlot(codeProto, ATOM_assemble, &source);
    
    // Assemble the blob
    
    // Go one pass thru and determine the number of bytes needed
    count = arrayCount(source);
    
    size = 0;
    for (i = 0; i < count; i++) {
		
	arrayGet(source, i, &assembly);
	
	// Get the data and the op code

	objectGetSlot(assembly, ATOMop, &opCode);

	if (opCode == (Proto) ATOMslotName) {
	    continue;
	}
	else if (opCode == (Proto) ATOMnop) {
	    size++;
	    continue;
	}
	else if (opCode == (Proto) ATOMbreak) {
	    size++;
	    continue;
	}
	else if (opCode == (Proto) ATOMpop) {
	    size++;
	    continue;
	}
	else if (opCode == (Proto) ATOMswap) {
	    size++;
	    continue;
	}
	else if (opCode == (Proto) ATOMcall) {
	    size++;
	    continue;
	}
	else if (opCode == (Proto) ATOMsetupFrame) {
	    size += 5;
	    continue;
	}
	else if (opCode == (Proto) ATOMrestoreFrame) {
	    size++;
	    continue;
	}
	else if (opCode == (Proto) ATOMpushSelf) {
	    size++;
	    continue;
	}
	else if (opCode == (Proto) ATOMreturn) {
	    size += 5;
	    continue;
	}
	else if (opCode == (Proto) ATOMpushimmediate) {
	    size += 5;
	    continue;
	}
	else if (opCode == (Proto) ATOMargCountCheck) {
	    size += 5;
	    continue;
	}
	else if (opCode == (Proto) ATOMbranch) {
	    size += 5;
	    continue;
	}
	else if (opCode == (Proto) ATOMbranchIfTrue) {
	    size += 5;
	    continue;
	}
	else if (opCode == (Proto) ATOMbranchIfFalse) {
	    size += 5;
	    continue;
	}

	printString("Bad Opcode ");
	printString(atomToString((Atom)opCode));
	printString(" \n");


	exit(1);
    }
    
    
    // Build a Blob
    codeBlob = objectNewBlob(size);
    p = (uchar *) objectPointerValue(codeBlob);


    // Now assemble the codes into the blob
    i = 0;

    for (i = 0; i < count; i++) {
	
	arrayGet(source, i, &assembly);
    
	objectGetSlot(assembly, ATOMop, &opCode);

	if (opCode == (Proto) ATOMslotName) {
	    objectGetSlot(assembly, ATOMoperand, &operand);
	    objectSetSlot(codeProto, ATOM_Name_, PROTO_ACTION_GET, (uint32) operand);
	    continue;
	}
	else if (opCode == (Proto) ATOMnop) {
	    *p = 0x01;
	    p++;
	    continue;
	}
	else if (opCode == (Proto) ATOMbreak) {
	    *p = 0x00;
	    p++;
	    continue;
	}
	else if (opCode == (Proto) ATOMpop) {
	    *p = 0x06;
	    p++;
	    continue;
	}
	else if (opCode == (Proto) ATOMswap) {
	    *p = 0x07;
	    p++;
	    continue;
	}
	else if (opCode == (Proto) ATOMrestoreFrame) {
	    *p = 0x09;
	    p++;
	    continue;
	}
	else if (opCode == (Proto) ATOMpushSelf) {
	    *p = 0x0b;
	    p++;
            continue;
	}
	else if (opCode == (Proto) ATOMreturn) {
	    *p = 0x05;
	    p++;

	    // simpler integer operand
	    objectGetSlot(assembly, ATOMoperand, &operand);
	    j = objectIntegerValue(operand);

	    *((uint32 *)p) = host2stdInteger(j);
	    
	    p += 4;
	    continue;
	}
	else if (opCode == (Proto) ATOMsetupFrame) {
	    *p = 0x08;
	    p++;

	    // simple integer operand
	    objectGetSlot(assembly, ATOMoperand, &operand);
	    j = objectIntegerValue(operand);

	    *((uint32 *)p) = host2stdInteger(j);
	    
	    p += 4;
	    continue;
	}
	else if (opCode == (Proto) ATOMpushimmediate) {
	    *p = 0x04;
	    p++;


	    // Make the compiler happy
	    j = (uint32) nil;
	    
	    // types should be activateSlot, self, string, or integer
	    objectGetSlot(assembly, ATOMtype, &opType);
	    
	    if (opType == (Proto) ATOMobject) {
		objectGetSlot(assembly, ATOMoperand, &tmpObj );
		j = (uint32) tmpObj;
		if ( (tmpObj != nil) && (PROTO_REFTYPE(tmpObj) == PROTO_REF_PROTO) ) {
		    k = objectGetSlot(tmpObj, ATOM_assemble, &tmp2);
		    if (k) {
			// Build the block and return a BLOCK reference
			j = (uint32) assemble(tmpObj, (Atom) nil, 1);
		    }
		}
	    }
	    

	    *((uint32 *)p) = host2stdInteger(j);
	    
	    p += 4;
	    continue;
	}
	else if (opCode == (Proto) ATOMargCountCheck) {
	    *p = 0x0c;
	    p++;

	    // types should be activateSlot, self, string, or integer
	    objectGetSlot(assembly, ATOMtype, &opType);
	    if (opType == (Proto) ATOMself) {
		// Invalid
                assert(0);
	    }
	    else if (opType == (Proto) ATOMactivateSlot) {
		// Invalid
                assert(0);
	    }
	    else if (opType == (Proto) ATOMobject) {
		// Invalid
		objectGetSlot(assembly, ATOMoperand, &tmpObj );
		j = objectIntegerValue(tmpObj);
	    } else {
		j = (uint32) nil;
	    }
	    
	    
	    *((uint32 *)p) = host2stdInteger(j);
	    
	    p += 4;
	    continue;
	}
	else if (opCode == (Proto) ATOMcall) {
	    *p = 0x02;
	    p++;
	    continue;
	}
	else if (opCode == (Proto) ATOMbranch) {
	    *p = 0x0d;
	    p++;

	    // simple integer operand
	    objectGetSlot(assembly, ATOMoperand, &operand);
	    offset = objectIntegerValue(operand);

	    *((uint32 *)p) = host2stdInteger(offset);
	    
	    p += 4;
	    continue;
	}
	else if (opCode == (Proto) ATOMbranchIfTrue) {
	    *p = 0x0e;
	    p++;

	    // simple integer operand
	    objectGetSlot(assembly, ATOMoperand, &operand);
	    offset = objectIntegerValue(operand);

	    *((uint32 *)p) = host2stdInteger(offset);
	    
	    p += 4;
	    continue;
	}
	else if (opCode == (Proto) ATOMbranchIfFalse) {
	    *p = 0x0f;
	    p++;

	    // simple integer operand
	    objectGetSlot(assembly, ATOMoperand, &operand);
	    offset = objectIntegerValue(operand);

	    *((uint32 *)p) = host2stdInteger(offset);
	    
	    p += 4;
	    continue;
	}

	exit(1);
    }
    

    // Set the '_code_' slot to be the code Blob
    objectSetSlot(codeProto, ATOM_code_, PROTO_ACTION_GET, (uint32) codeBlob);

    // Set the '_Assembly' slot to be the code Blob
    //DRUDRU - Should I rename the proto?
    ////objectSetSlot(codeProto, ATOM_assembly, PROTO_ACTION_GET, (uint32) source);
    
    if (isBlock) {
	codeProto = objectSetAsBlockProto(codeProto);
	return codeProto;
    } else {
	// Set the proto slot to be a passthru to the 
	// code proto
	codeProto = objectSetAsCodeProto(codeProto);
	objectSetSlot(proto, slot, PROTO_ACTION_PASSTHRU, (uint32) codeProto);
    }

    return nil;
}


// Assemble all
// This walks a soup and 
Proto assembleAll (Proto proto)
{
    int i,j,k;
    struct _protoEntry * slotPtr;
    Proto p1;
    Proto tmp, tmp2;



    // Determine the type of reference this is
    i = (uint32) proto & PROTO_REF_TYPE;
    p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);

    if (proto == nil) {
	return proto;
    }
    

    //// In theory, a switch doesn't need to be here.
    //// we should put in asserts()
    switch (i) {
	case PROTO_REF_INT:
	    break;
	case PROTO_REF_PROTO:
	    // If this is a forward
	    if (PROTO_ISFORWARDER(p1)) {
		assembleAll(PROTO_FORWARDVALUE(p1));
	    } else {
		// enumerate through the proto showing it's contents
		// NOTE - since assemble affects the proto that is
		// pointed to by the slot in this proto... we can
		// manipulate it without the worry that we will affect
		// the enumerating process
		j = 0;
		while (1) {
		    slotPtr = objectEnumerate(proto, &j, 1);
		    if (slotPtr == nil) {
			break;	// The end of the enumerate
		    }

		    tmp = protoSlotData(proto, slotPtr);
		    i = (uint32) tmp & PROTO_REF_TYPE;
	
		    k = slotAction(slotPtr);
		    switch (k) {
			case PROTO_ACTION_GET:
			    if ((PROTO_REF_PROTO == i) && (nil != tmp)) {
				i = objectGetSlot(tmp, ATOM_assemble, &tmp2);
				if (i) {
				    assemble(proto, (Atom) slotKey(slotPtr), 0);
				} else {
				    assembleAll(tmp);
				}
			    }
			    break;
			case PROTO_ACTION_PASSTHRU:
			case PROTO_ACTION_SET:
			case PROTO_ACTION_GETFRAME:
			case PROTO_ACTION_SETFRAME:
			    break;
		    }
		    
		}
	    }
	    
	    
	    break;
	case PROTO_REF_BLOB:
	    break;
	case PROTO_REF_ATOM:
	    break;
    }

    return proto;
    
}
