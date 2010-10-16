
//
// while.c
//
// <see corresponding .h file>
// 
//
// $Id: while.c,v 1.4 2001/11/16 08:19:42 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "array.h"


//
//  whileObj - build a code object, and return it
// 

Proto whileObj(int isTrueLoop)
{

    Proto codeObj;
    Proto assembly;
    Proto tmp;


    codeObj = objectNew(0);

    // Setup the arguments

    // This points to the target
    objectSetSlot(codeObj, stringToAtom("a"), PROTO_ACTION_GETFRAME, (uint32) objectNewInteger(6));
    // This one points to arg0
    objectSetSlot(codeObj, stringToAtom("b"), PROTO_ACTION_GETFRAME, (uint32) objectNewInteger(8));
    
    assembly = createArray( (Proto) stringToAtom("Ignore This Slot"));
    objectSetSlot(codeObj, stringToAtom("_assemble"), PROTO_ACTION_GET, (uint32) assembly);
    
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("slotName"));
    if (isTrueLoop) {
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("whileTrue:"));
    } else {
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("whileFalse:"));
    }
    addToArray(assembly, tmp);
    
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("setupFrame"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(0));
    addToArray(assembly, tmp);

    // Push value
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("value"));
    addToArray(assembly, tmp);
    

    // Now discover 'a'
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("a"));
    addToArray(assembly, tmp);

    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushSelf"));
    addToArray(assembly, tmp);
    
    // Push the arg count
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(2));
    addToArray(assembly, tmp);
    
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("call"));
    addToArray(assembly, tmp);
    


    // With 'a''s value on the stack and 'value', evaluate that
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(2));
    addToArray(assembly, tmp);
    
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("call"));
    addToArray(assembly, tmp);


    // Now do the branch
    tmp = objectNew(0);
    if (isTrueLoop) {
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("branchIfFalse"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(29));
    } else {
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("branchIfTrue"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(29));
    }
    addToArray(assembly, tmp);





    // If branch not taken...

    // Push value
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("value"));
    addToArray(assembly, tmp);
    
    // Now discover 'b'
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("b"));
    addToArray(assembly, tmp);

    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushSelf"));
    addToArray(assembly, tmp);
    
    // Push the arg count
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(2));
    addToArray(assembly, tmp);
    
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("call"));
    addToArray(assembly, tmp);
    
    // With 'b''s value on the stack and 'value', evaluate that
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(2));
    addToArray(assembly, tmp);
    
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("call"));
    addToArray(assembly, tmp);

    // Remove the result

    //// In the future we will use this result to do 'continue/break'
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pop"));
    addToArray(assembly, tmp);

    // Now do the branch to the top, to do it all over again
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("branch"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(-57));
    addToArray(assembly, tmp);





    // The end
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("restoreFrame"));
    addToArray(assembly, tmp);
    
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("return"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(2));
    addToArray(assembly, tmp);


    return codeObj;
}

