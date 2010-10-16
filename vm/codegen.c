
//
// codegen.c
//
// <see corresponding .h file>
// 
//
// $Id: codegen.c,v 1.16 2002/01/28 23:15:22 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "array.h"
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

// Globals

char stringBuffer[256];


// Function Prototypes

Proto transform(Proto parseTree);

Proto simplifyParseTree(Proto parseTree);

//Proto simplifyExpression(Proto parseTree);

Proto simplifySlots(Proto parseTree);

Proto cleanCodeBlocks(Proto parseTree);

Proto setupBinarySlots(Proto parseTree);

Proto setupKeywordSlots (Proto parseTree);

Proto setupArgSlots(Proto parseTree);

Proto setupMethods(Proto parseTree);

Proto setupBlocks(Proto parseTree);

Proto setupLocals(Proto parseTree);

void countArgsLocals(Proto obj, int *, int *);


// The Big Function
Proto realCodeGen(Proto parseTree);

// Higher Granularity Codegen functions
Proto cgBuildObject(Proto parseTree);

//void  cgBuildSlotList(Proto parseTree, Proto objectUnderConstruction);

void  cgBuildSlot(Proto parseTree, Proto newObj);

Proto cgBuildArray(Proto parseTree);

Proto cgBuildReceiver(Proto parseTree);

Proto cgBuildReceiverAssembly (Proto parseTree);

Proto cgBuildExpression (Proto parseTree);

Proto cgBuildCodeBlock(Proto parseTree, Proto annotation);

Proto cgBuildMessage(Proto parseTree);

Proto cgBuildUnary(Proto proto);

Proto cgBuildBinary(Proto proto);



//
//  transform - do semantic checks, and simplifications of tree to get it ready for
//  the code generator
// 

Proto transform (Proto parseTree)
{
    Proto newTree;
    
    // Walk the tree
    // and then simplify all arrays that are 0 length

    parseTree = removeForwards(parseTree);

    if (debugParse && (debugParseLevel == 0) ) {
	printString("***No Transform ***\n");
	objectDump(parseTree);
    }

    newTree = simplifyParseTree(parseTree);
    if (debugParse && (debugParseLevel == 1 || debugParseLevel == 0)) {
	printString("*** SimplifyParse ***\n");
	objectDump(newTree);
    }

    newTree = simplifySlots(parseTree);
    if (debugParse && (debugParseLevel == 2 || debugParseLevel == 0)) {
	printString("*** SimplifySlots ***\n");
	objectDump(newTree);
    }

    newTree = cleanCodeBlocks(newTree);
    if (debugParse && (debugParseLevel == 3 || debugParseLevel == 0)) {
	printString("*** CleanCodeBlocks ***\n");
	objectDump(newTree);
    }

    newTree = setupBinarySlots(newTree);
    if (debugParse && (debugParseLevel == 4 || debugParseLevel == 0)) {
	printString("*** MethodBinarySlot ***\n");
	objectDump(newTree);
    }
    newTree = removeForwards(newTree);    

    newTree = setupKeywordSlots(newTree);
    if (debugParse && (debugParseLevel == 5 || debugParseLevel == 0)) {
	printString("*** MethodKeywordSlot ***\n");
	objectDump(newTree);
    }
    newTree = removeForwards(newTree);    

    newTree = setupArgSlots(newTree);
    if (debugParse && (debugParseLevel == 6 || debugParseLevel == 0)) {
	printString("*** setupArgSlot ***\n");
	objectDump(newTree);
    }
    newTree = removeForwards(newTree);    

    newTree = setupMethods(newTree);
    if (debugParse && (debugParseLevel == 7 || debugParseLevel == 0)) {
	printString("*** MethodAnnotate ***\n");
	objectDump(newTree);
    }
    newTree = removeForwards(newTree);    

    newTree = setupBlocks(newTree);
    if (debugParse && (debugParseLevel == 8 || debugParseLevel == 0)) {
 	printString("*** BlockAnnotate ***\n");
 	objectDump(newTree);
    }
    newTree = removeForwards(newTree);    
    
    newTree = setupLocals(newTree);
    if (debugParse && (debugParseLevel == 8 || debugParseLevel == 0)) {
 	printString("*** SetupLocals ***\n");
 	objectDump(newTree);
    }
    newTree = removeForwards(newTree);    
    return newTree;
    
}



//
//  simplify - eliminate the fluff of the parsetree, arrays with no values
// 

Proto simplifyParseTree(Proto parseTree)
{

    int i;
    int size;
    Proto tmp;
    Proto tmpObj;
    Proto proto = parseTree;
    
    // In general, right after a parse most expressions have 
    // lots of arrays and subarrays that are empty. (Because
    // the parse step will match from the topmost production
    // down through the productions to the terminals.)
    // So, we need to simplify the parse tree so the codegen
    // is a lot easier.


    // Walk the parse tree depth first
    // If we encounter Message, Binary, or Unary arrays that have a zero length,
    // then we remove them


    // ACTUAL CODE BELOW HERE

    // Go down each proto, and through each slot.
    // if it hits an expression, go into expression compress mode
    
    // Determine the type of reference this is
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    switch (i) {
	case PROTO_REF_PROTO:
	    if (PROTO_ISFORWARDER(proto)) {
	        assert(0);
	    }
	    
	    size = arrayCount(proto);
	    
	    // enumerate through the proto showing it's contents
	    for (i = 0; i < size; i++) {
		// Access the data and set it to the new value
		arrayGet(proto, i, &tmp);
		tmp = simplifyParseTree(tmp);
		arraySet(proto, i, tmp);
	    }
	    

            // Expressions always only have 1 child array
	    objectGetSlot(proto, stringToAtom("name"), &tmp);

	    if ( (tmp == (Proto) stringToAtom("Message")) && (1 == size) ) {
		arrayGet(proto, 0, &tmpObj);
		return tmpObj;
	    }

	    if ( (tmp == (Proto) stringToAtom("Binary")) && (1 == size) ) {
		arrayGet(proto, 0, &tmpObj);
		return tmpObj;
	    }

	    if ( (tmp == (Proto) stringToAtom("Unary")) && (1 == size) ) {
		arrayGet(proto, 0, &tmpObj);
		return tmpObj;
	    }

	    return proto;

	    break;
    }

    // Make the compiler happy
    return proto;
    
}


//
//  simplifySlots   - Remove the zero length expressions from slots
// 

Proto simplifySlots (Proto parseTree)
{

    int i,j;
    int isSimpleSlot = 0;
    Proto p1;
    Proto tmp;
    Proto tmp2;
    Proto proto = parseTree;
    

    // Determine the type of reference this is
    p1 = PROTO_REFVALUE(proto);
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    if (PROTO_REF_ATOM == i) {
	return proto;
    }

    if (PROTO_REF_PROTO != i) {
    	assert(0);
    }

    if (PROTO_ISFORWARDER(proto)) {
        assert(0);
    }

    
    j = arrayCount(proto);

    objectGetSlot(proto, stringToAtom("name"), &tmp);
    if (tmp == (Proto) stringToAtom("Slot")) {
	arrayGet(proto, 0, &tmp);
// 	if ( (Proto) stringToAtom("Identifier-Slot") == tmp) {
// 	    isSimpleSlot = 0;
// 	}
	if ( (Proto) stringToAtom("Data-Get-Slot") == tmp) {
	    isSimpleSlot = 1;
	}
	if ( (Proto) stringToAtom("Data-SetGet-Slot") == tmp) {
	    isSimpleSlot = 1;
	}
    }

    // enumerate through the proto

    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmp);
	simplifySlots(tmp);
    }
    
    if (isSimpleSlot) {
	// Get the contained receiver 
	arrayGet(proto, 2, &tmp);

	// Now get the receiver (notice I'm reusing 'tmp'
	arrayGet(tmp, 0, &tmp2);

 	// Now set the receiver
	arraySet(proto, 2, tmp2);
    }

    // Make the compiler happy
    return proto;
}





//
//  cleanCodeBlocks - eliminate the codeBlocks with just the "Nill Expression"
//                    statements. Also, remove the 'Nil Expression' arrays
//                    from valid codeBlocks.
// 

Proto cleanCodeBlocks (Proto parseTree)
{

    int i,j;
    int arrayWasNil = 0;
    //Proto p1;
    Proto codeBlock;
    Proto tmp;
    Proto tmpObj;
    Proto tmpName;
    Proto proto = parseTree;

    
    // Go down each proto, and through each slot.
    // if it is an object, it should check for a codeBlock
    // if the codeblock just has a 'Nil Expression' then
    // it should just remove it. Otherwise, it should
    // remove the last 'Nil Expression'
    
    //p1 = PROTO_REFVALUE(proto);
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    if (PROTO_REF_ATOM == i) {
	return proto;
    }

    if (PROTO_REF_PROTO != i) {
    	assert(0);
    }

    if (PROTO_ISFORWARDER(proto)) {
        assert(0);
    }


    // Enumerate through the proto, recursively 
    // calling this routine

    j = arrayCount(proto);

    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmp);
	cleanCodeBlocks(tmp);
    }

    // On the way up, after the routine has finished
    // with the arrays at the bottom of the tree (leaves)
    // We do the real work
    assert(objectGetSlot(proto, stringToAtom("name"), &tmp));
    
    if (tmp == (Proto) stringToAtom("Object")) {
	j = 0;
	assert(arrayGet(proto, 0, &codeBlock));
	// If the '0' slot isn't the 'CodeBlock', then the '1' slot
	// is...
	assert(objectGetSlot(codeBlock, stringToAtom("name"), &tmp));
	if (tmp == (Proto) stringToAtom("SlotList")) {
	    assert(arrayGet(proto, 1, &codeBlock));
	    j = 1;
	}

	// Get the last entry in the code block
	i = arrayCount(codeBlock);
	i--;
	
	
	// All this does is verify that the last entry/expression is 
	// a receiver of a "Nil Expression"
	arrayGet(codeBlock, i, &tmpObj);
	assert(objectGetSlot(tmpObj, stringToAtom("name"), &tmpName));
	if (tmpName == (Proto) stringToAtom("Expression")) {
	    assert(objectGetSlot(tmpObj, stringToAtom("size"), &tmp));
	    if (1 == objectIntegerValue(tmp)) {
		assert(arrayGet(tmpObj, 0, &tmp));
		assert(objectGetSlot(tmp, stringToAtom("name"), &tmpName));
		if (tmpName == (Proto) stringToAtom("Receiver")) {
		    assert(arrayGet(tmp, 0, &tmpName));
		    if (tmpName == (Proto) stringToAtom("Nil Expression")) {
			removeFromArray(codeBlock, i);
			// assert(objectDeleteSlot(codeBlock, stringToAtom(buff)));
			arrayWasNil = 1;
		    }
		}
	    }
	}
	
	// If that was the only expression in the codeBlock, then the 
	// entire codeBlock should be removed.
	if ((0 == i) && (arrayWasNil)) {
	    removeFromArray(proto, j);
	}
    }
    

    // Make the compiler happy
    return proto;
}

//
//  setupBinarySlots - Make the parameters to a binary slot actual members in a slot
//                     list for the object.
// 

Proto setupBinarySlots (Proto parseTree)
{

    int i,j;
    int isObjectBinarySlot = 0;
    Proto p1;
    Proto tmp;
    Proto tmp2;
    Proto slotName;
    Proto argName;
    Proto privateTree;
    Proto newArray;
    Proto proto = parseTree;
    Proto codeObj;
    

    // Determine the type of reference this is
    p1 = PROTO_REFVALUE(proto);
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    if (PROTO_REF_ATOM == i) {
	return proto;
    }

    if (PROTO_REF_PROTO != i) {
    	assert(0);
    }

    if (PROTO_ISFORWARDER(proto)) {
        assert(0);
    }

    j = arrayCount(proto);

    objectGetSlot(proto, stringToAtom("name"), &tmp);
    if (tmp == (Proto) stringToAtom("Slot")) {
	arrayGet(proto, 0, &tmp);
	if (tmp == (Proto) stringToAtom("Binary-Slot")) {
	    isObjectBinarySlot = 1;
	}
    }

    // enumerate through the proto
    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmp);
	setupBinarySlots(tmp);
    }
    
    if (isObjectBinarySlot) {
	// Get the 'Binary-Slot'
	arrayGet(proto, 1, &tmp);
	
	// Get the slotName
	arrayGet(tmp, 0, &slotName);
	
	// Get the argName
	arrayGet(tmp, 1, &argName);
	
	// Get the 'Object'
	arrayGet(tmp, 3, &codeObj);

	newArray = createArray( (Proto) stringToAtom("Slot"));
	addToArray(newArray, (Proto) stringToAtom("Arg-Slot"));
	addToArray(newArray, argName);
	
	// Also add the position in the stack frame for the argument
	addToArray(newArray, (Proto) stringToAtom("8"));

	// See if there is already a slot list
	arrayGet(codeObj, 0, &tmp);
	objectGetSlot(tmp, stringToAtom("name"), &tmp2);

	if (tmp2 == (Proto) stringToAtom("SlotList")) {
	    addToArray(tmp, newArray);
	} else {
	    privateTree = createArray( (Proto) stringToAtom("SlotList"));
	    addToArray(privateTree, newArray);
	    arrayInsertObjAt(codeObj, privateTree, 0);
	}
    }

    // Make the compiler happy
    return proto;
}


//
//  setupKeywordSlots - Make the parameters to a keyword slot actual members in a slot
//                      list for the object.
// 

Proto setupKeywordSlots (Proto parseTree)
{

    int i,j;
    int isKeywordSlot = 0;
    Proto p1;
    Proto tmp;
    Proto tmp2;
    Proto slotName;
    Proto argName;
    Proto privateTree;
    Proto newArray;
    Proto proto = parseTree;
    Proto codeObj;
    Proto words;
    int   argPosition;
    int   count;
    Proto keyObj;
    char  buff[8];
    
    

    // Determine the type of reference this is
    p1 = PROTO_REFVALUE(proto);
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    if (PROTO_REF_ATOM == i) {
	return proto;
    }

    if (PROTO_REF_PROTO != i) {
    	assert(0);
    }

    if (PROTO_ISFORWARDER(proto)) {
        assert(0);
    }

    j = arrayCount(proto);

    objectGetSlot(proto, stringToAtom("name"), &tmp);
    if (tmp == (Proto) stringToAtom("Slot")) {
	arrayGet(proto, 0, &tmp);
	if (tmp == (Proto) stringToAtom("Keyword-Slot")) {
	    isKeywordSlot = 1;
	}
    }

    // enumerate through the proto
    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmp);
	setupKeywordSlots(tmp);
    }
    
    if (isKeywordSlot) {
	// Get the 'KeywordSlot'
	arrayGet(proto, 1, &keyObj);
	
	// Loop through to build the slot name and the argument names

	words = createArray( (Proto) stringToAtom("wordlist") );
	
	argPosition = 8;
	
	count = arrayCount(keyObj);
	count--;
	// Don't need the last two items
	count--;
	count--;
	
	// Get the 'Object', which will be used later
	arrayGet(keyObj, (count+2), &codeObj);

	i = 0;
	while (1) {
	    // Get the slotName
	    arrayGet(keyObj, i, &slotName);
	    addToArray(words, slotName);
	    i++;
	    
	    arrayGet(keyObj, i, &tmp2);
	    assert(tmp2 == (Proto) stringToAtom("Identifier"));
	    i++;

	    // Get the argName
	    arrayGet(keyObj, i, &argName);
	    i++;
	
	    newArray = createArray( (Proto) stringToAtom("Slot"));
	    addToArray(newArray, (Proto) stringToAtom("Arg-Slot"));
	    addToArray(newArray, argName);
	    
	    // Also add the position in the stack frame for the argument
	    sprintf(buff, "%d", argPosition);
	    argPosition++;
	    addToArray(newArray, (Proto) stringToAtom(buff));
	    
	    // See if there is already a slot list
	    arrayGet(codeObj, 0, &tmp);
	    objectGetSlot(tmp, stringToAtom("name"), &tmp2);
	    
	    if (tmp2 == (Proto) stringToAtom("SlotList")) {
		addToArray(tmp, newArray);
	    } else {
		privateTree = createArray( (Proto) stringToAtom("SlotList"));
		addToArray(privateTree, newArray);
		arrayInsertObjAt(codeObj, privateTree, 0);
	    }

	    // End the loop
	    if (i > count) {
		break;
	    }
	}

	count = arrayCount(words);
	stringBuffer[0] = 0;
	for (i = 0; i < count; i++) {
	    arrayGet(words, i, &tmp);
	    strcat(stringBuffer, atomToString((Atom) tmp));
	}
	// Annotate the object
	annotateArray(keyObj, stringToAtom("slotName"), (Proto) stringToAtom(stringBuffer));
    }

    // Make the compiler happy
    return proto;
}


//
//  setupArgSlots - Give the argument slots proper integer positions on the stack
// 

Proto setupArgSlots (Proto parseTree)
{
    int i,j;
    int isSlotList = 0;
    Proto p1;
    Proto tmp;
    Proto tmp2;
    Proto proto = parseTree;
    Proto slot;
    char * p;
    int count;
    char buff[8];
    
    

    // Determine the type of reference this is
    p1 = PROTO_REFVALUE(proto);
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    if (PROTO_REF_ATOM == i) {
	return proto;
    }

    if (PROTO_REF_PROTO != i) {
    	assert(0);
    }

    if (PROTO_ISFORWARDER(proto)) {
        assert(0);
    }

    j = arrayCount(proto);

    // Tag the arg slots
    objectGetSlot(proto, stringToAtom("name"), &tmp);
    if (tmp == (Proto) stringToAtom("SlotList")) {
	isSlotList = 1;
    }

    // enumerate through the proto
    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmp);
	setupArgSlots(tmp);
    }
    
    if (isSlotList) {
	count = 8;
	for (i = 0; i < j; i++) {
	    arrayGet(proto, i, &slot);
	    objectGetSlot(slot, stringToAtom("name"), &tmp2);
	    if (tmp2 == (Proto) stringToAtom("Slot")) {
		arrayGet(slot, 0, &tmp2);
		if (tmp2 == (Proto) stringToAtom("Arg-Slot")) {
		    arrayGet(slot, 1, &tmp2);
		    p = atomToString((Atom) tmp2);
		    sprintf(buff, "%d", count);
		    if (p[0] == ':') {
			p++;	// Get rid of the colon
			arraySet(slot, 1, (Proto) stringToAtom(p));
			addToArray(slot,  (Proto) stringToAtom(buff));
		    } else {
			arraySet(slot, 2, (Proto) stringToAtom(buff));
		    }
		    count++;
		}
	    }
	}
    }
    
    // Make the compiler happy
    return proto;
}



//
//  setupMethods    - Add the slotName and hopefully the locals and param counts
//                    to the Code Object arrays
// 

Proto setupMethods (Proto parseTree)
{

    int i,j;
    int isObjectUnarySlot   = 0;
    int isObjectBinarySlot  = 0;
    int isObjectKeywordSlot = 0;
//    int isBlock            = 0;
    Proto p1;
    Proto tmp;
//    Proto tmp2;
    Proto slotName;
    Proto proto = parseTree;
    Proto codeObj;
    Proto annotate;
    
    int paramCount, localCount;
    

    // Determine the type of reference this is
    p1 = PROTO_REFVALUE(proto);
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    if (PROTO_REF_ATOM == i) {
	return proto;
    }

    if (PROTO_REF_PROTO != i) {
    	assert(0);
    }

    if (PROTO_ISFORWARDER(proto)) {
        assert(0);
    }


    j = arrayCount(proto);

    objectGetSlot(proto, stringToAtom("name"), &tmp);
    if (tmp == (Proto) stringToAtom("Slot")) {
	arrayGet(proto, 0, &tmp);
	if (tmp == (Proto) stringToAtom("Unary-Slot")) {
	    isObjectUnarySlot = 1;
	}
	if (tmp == (Proto) stringToAtom("Binary-Slot")) {
	    isObjectBinarySlot = 1;
	}
	if (tmp == (Proto) stringToAtom("Keyword-Slot")) {
	    isObjectKeywordSlot = 1;
	}
    }

    // enumerate through the proto
    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmp);
	setupMethods(tmp);
    }
    
    if (isObjectUnarySlot) {

	paramCount = 0;
	localCount = 0;
	
	// Get the slot name
	arrayGet(proto, 1, &slotName);
	
	// Get the contained object 
	arrayGet(proto, 2, &tmp);
	
	// Get the UnarySlot (not the Unary-Slot) proto
	// it's '1' slot is the 'Object'
	arrayGet(tmp, 1, &codeObj);
	
	// Count the parameters and locals
	countArgsLocals(codeObj, &paramCount, &localCount);
	// There shouldn't be any parameters
	assert(!paramCount);
	
	
	// Annotate the CodeBlock
	annotateArray(codeObj, stringToAtom("slotName"), slotName);
	annotateArray(codeObj, stringToAtom("localCount"), objectNewInteger(localCount));
	annotateArray(codeObj, stringToAtom("paramCount"), objectNewInteger(paramCount));
    }
    else if (isObjectBinarySlot) {
	paramCount = 1;
	localCount = 0;
	
	// Get the 'Binary-Slot'
	arrayGet(proto, 1, &tmp);
	
	// Get the slotName
	arrayGet(tmp, 0, &slotName);
	
	// Get the 'Object'
	arrayGet(tmp, 3, &codeObj);

	// Count the parameters and locals
	countArgsLocals(codeObj, &paramCount, &localCount);
	// There shouldn't be any parameters defined in the object
	// it is defined in the initializer
	assert((paramCount == 1));

	// Annotate the CodeBlock
	annotateArray(codeObj, stringToAtom("slotName"), slotName);
	annotateArray(codeObj, stringToAtom("localCount"), objectNewInteger(localCount));
	annotateArray(codeObj, stringToAtom("paramCount"), objectNewInteger(paramCount));
    }
    else if (isObjectKeywordSlot) {
	paramCount = 0;
	localCount = 0;
	
	// Get the 'KeywordSlot'
	arrayGet(proto, 1, &tmp);
	
	i = arrayCount(tmp);
	i--;
	assert(objectGetSlot(tmp, stringToAtom("_annotate"), &annotate));
	assert(objectGetSlot(annotate, stringToAtom("slotName"), &slotName));

	arrayGet(tmp, i, &codeObj);
	
	// Count the parameters and locals
	countArgsLocals(codeObj, &paramCount, &localCount);

	// Annotate the CodeBlock
	annotateArray(codeObj, stringToAtom("slotName"), slotName);
	annotateArray(codeObj, stringToAtom("localCount"), objectNewInteger(localCount));
	annotateArray(codeObj, stringToAtom("paramCount"), objectNewInteger(paramCount));
    }
    
    // Make the compiler happy
    return proto;
}


//
//  setupBlocks     - Add the slotName and hopefully the locals and param counts
//                    to the Code Object arrays
// 

Proto setupBlocks (Proto parseTree)
{

    int i,j;
    int isBlock            = 0;
    Proto p1;
    Proto tmp;
    Proto tmp2;
    Proto proto = parseTree;
    Proto objectCount;
    Proto blockObj;
    
    int paramCount, localCount;
    

    // Determine the type of reference this is
    p1 = PROTO_REFVALUE(proto);
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    if (PROTO_REF_ATOM == i) {
	return proto;
    }

    if (PROTO_REF_PROTO != i) {
    	assert(0);
    }

    if (PROTO_ISFORWARDER(proto)) {
        assert(0);
    }

    objectGetSlot(proto, stringToAtom("size"), &objectCount);
    j = objectIntegerValue(objectCount);

    // A fairly complex way of determining if a codeblock is part of
    // a block or a method
    objectGetSlot(proto, stringToAtom("name"), &tmp);
    if (tmp == (Proto) stringToAtom("Receiver")) {
	arrayGet(proto, 0, &tmp);
	if (tmp == (Proto) stringToAtom("Object")) {
	    arrayGet(proto, 1, &tmp2);
	    i = arrayCount(tmp2);
	    if (i != 0) {
		i--;
		arrayGet(tmp2, i, &tmp);
		objectGetSlot(tmp, stringToAtom("name"), &tmp2);
		if (tmp2 == (Proto) stringToAtom("CodeBlock")) {
		    isBlock = 1;
		}
	    }
	}
    }
    
		    
    // enumerate through the proto
    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmp);
	setupBlocks(tmp);
    }
    
    if (isBlock) {
	
	paramCount = 0;
	localCount = 0;
	
	// Get the Object array
	arrayGet(proto, 1, &blockObj);

	// Count the parameters and locals
	countArgsLocals(blockObj, &paramCount, &localCount);
	
	// Annotate the CodeBlock
	annotateArray(blockObj, stringToAtom("slotName"), (Proto) stringToAtom("**Block"));
	annotateArray(blockObj, stringToAtom("localCount"), objectNewInteger(localCount));
	annotateArray(blockObj, stringToAtom("paramCount"), objectNewInteger(paramCount));
    }

    
    // Make the compiler happy
    return proto;
}



//
//  setupLocals     - Transform the slots of the code protos so that the slots
//                    for the local variables they are locals
//                    
// 

Proto setupLocals (Proto parseTree)
{

    int i,j;
    int isCode            = 0;
    Proto p1;
    Proto tmp;
    Proto tmp2;
    Proto proto = parseTree;
    Proto objectCount;
    int framePos;
    Proto localCount;
    Proto annotate;
    int count;
    Proto objects;
    Proto slotList;
    Proto newSlot;
    char buff[16];
    Proto tmp3;
    

    // Determine the type of reference this is
    p1 = PROTO_REFVALUE(proto);
    i  = PROTO_REFTYPE(proto);

    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    if (PROTO_REF_ATOM == i) {
	return proto;
    }

    if (PROTO_REF_PROTO != i) {
    	assert(0);
    }

    if (PROTO_ISFORWARDER(proto)) {
        assert(0);
    }

    objectGetSlot(proto, stringToAtom("size"), &objectCount);
    j = objectIntegerValue(objectCount);

    // A fairly complex way of determining if a codeblock is part of
    // a block or a method
    i = objectGetSlot(proto, stringToAtom("_annotate"), &tmp);
    if (i) {
        i = objectGetSlot(tmp, stringToAtom("localCount"), &localCount);
	if (i && (objectIntegerValue(localCount) > 0)) {
	    isCode = 1;
        }
    }	    
    
    // enumerate through the proto
    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmp);
	setupLocals(tmp);
    }
    
    if (isCode) {
        assert(objectGetSlot(proto, stringToAtom("_annotate"), &annotate));
	
        assert(objectGetSlot(annotate, stringToAtom("localCount"), &localCount));
	count = objectIntegerValue(localCount);

	// This is the position in the stack frame for the variables
	framePos = 0;

	
	objects = createArray(nil);
        objectSetSlot(annotate, stringToAtom("initialValues"), PROTO_ACTION_GET, (uint32) objects);
	
        assert(arrayGet(proto, 0, &slotList));
	j = arrayCount(slotList);
	

	for (i = 0; i < j; i++) {
	    arrayGet(slotList, i, &tmp);
	    arrayGet(tmp, 0, &tmp2);
	    if (tmp2 == (Proto) stringToAtom("Data-SetGet-Slot")) {
		newSlot = createArray((Proto) stringToAtom("Slot"));
		addToArray(newSlot, (Proto) stringToAtom("Local-SetGet-Slot"));
		// Add the name
		arrayGet(tmp, 1, &tmp2);
		addToArray(newSlot, tmp2);
		// Add the frame position
		sprintf(buff, "%d", framePos);
		addToArray(newSlot, (Proto) stringToAtom(buff));
		framePos = framePos - 1;
		// Set the new slot
		arraySet(slotList, i, newSlot);
		// Get the initial value for the value
		arrayGet(tmp, 2, &tmp2);
		tmp3 = cgBuildReceiver(tmp2);
		addToArray(objects, tmp3);
	    }
	    else if (tmp2 == (Proto) stringToAtom("Data-Get-Slot")) {
		newSlot = createArray((Proto) stringToAtom("Slot"));
		addToArray(newSlot, (Proto) stringToAtom("Local-Get-Slot"));
		// Add the name
		arrayGet(tmp, 1, &tmp2);
		addToArray(newSlot, tmp2);
		// Add the frame position
		sprintf(buff, "%d", framePos);
		addToArray(newSlot, (Proto) stringToAtom(buff));
		framePos = framePos - 1;
		// Set the new slot
		arraySet(slotList, i, newSlot);
		// Get the initial value for the value
		arrayGet(tmp, 2, &tmp2);
		tmp3 = cgBuildReceiver(tmp2);
		addToArray(objects, tmp3);
	    }
	    else if (tmp2 == (Proto) stringToAtom("Identifier-Slot")) {
		newSlot = createArray((Proto) stringToAtom("Slot"));
		addToArray(newSlot, (Proto) stringToAtom("Local-Get-Slot"));
		// Add the name
		arrayGet(tmp, 1, &tmp2);
		addToArray(newSlot, tmp2);
		// Add the frame position
		sprintf(buff, "%d", framePos);
		addToArray(newSlot, (Proto) stringToAtom(buff));
		framePos = framePos - 1;
		// Set the new slot
		arraySet(slotList, i, newSlot);
		// Get the initial value for the value
		addToArray(objects, nil);
	    }
	}
    }
 
    // Make the compiler happy
    return proto;
}


// This function will count the number of slots in an object in order
// to determine the number of local variables a method or block is using.
void countArgsLocals (Proto obj, int * argCount, int * localCount )
{
    Proto tmp;
    Proto tmp2;
    Proto tmp3;
    int i,j;
    
    *argCount = 0;
    *localCount = 0;
    

    // The first slot is always the slot or slotList
    arrayGet(obj, 0, &tmp);
		
    // Check to see if it is a SlotList
    objectGetSlot(tmp, stringToAtom("name"), &tmp2);
    if (tmp2 == (Proto) stringToAtom("SlotList")) {
	j = arrayCount(tmp);

	// enumerate through the proto
	for (i = 0; i < j; i++) {
	    arrayGet(tmp,  i, &tmp2);
	    arrayGet(tmp2, 0, &tmp3);
	    if (tmp3 == (Proto) stringToAtom("Arg-Slot")) {
		(*argCount)++;
	    } else {
		(*localCount)++;
	    }
	}
    }
}




//
//  codeGen - generate a final soup for this tree, this could get to be a slightly
//            big function
//            NOTE - even though this is called codeGen - that is only part of it's
//            functionality because a soup is composed of objects and code. I'm just
//            emphasizing the more difficult side of the function... the code generation
// 

Proto codeGen (Proto parseTree, int addStart)
{

    Proto tmp;


    Proto soup;
    Proto start;
    Proto root;
    
    Proto startCode;
    

    // Build the soup
    soup = realCodeGen(parseTree);

    // assert(0);
    
    if (addStart) {
	
	// Finish the soup with a little spice (i.e. this a slot that the vm 
	// will look for to start the program running.)
	start = objectNew(0);
	objectGetSlot(soup, stringToAtom("Root"), &root);
	objectSetSlot(root, stringToAtom("_Start"), PROTO_ACTION_GET, (uint32) start);
	
	objectSetSlot(start, stringToAtom("Stack"), PROTO_ACTION_GET, (uint32) objectNewInteger(2048));
	
	
	startCode = createArray( (Proto) stringToAtom("Ignore This Slot"));
	objectSetSlot(start, stringToAtom("_assemble"), PROTO_ACTION_GET, (uint32) startCode);
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("slotName"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("_Start"));
	addToArray(startCode, tmp);
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("setupFrame"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(0));
	addToArray(startCode, tmp);
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("nop"));
	addToArray(startCode, tmp);
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("#Start#"));
	addToArray(startCode, tmp);

	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom("Start:"));
	addToArray(startCode, tmp);
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushSelf"));
	addToArray(startCode, tmp);
	
	// Push the arg count
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(2));
	addToArray(startCode, tmp);
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("call"));
	addToArray(startCode, tmp);
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pop"));
	addToArray(startCode, tmp);
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("break"));
	addToArray(startCode, tmp);
    }
    
    return soup;
}

//
//  realCodeGen - generate a final soup for this tree, this could get to be a slightly
//            big function
// 

Proto realCodeGen(Proto parseTree)
{

    Proto tmpObj;
    Proto arrayName;
    int i;
    Proto proto = parseTree;

    

    // Begin the process of code generation...
        
    if (proto == nil) {
	printString(" ERROR a <nil> ");
	assert(0);
    }

    i = objectGetSlot(proto, stringToAtom("name"), &arrayName);
    assert(i);

    if ((Proto) stringToAtom("Soup") == arrayName) {
	arrayGet(proto, 0, &tmpObj);
	return cgBuildObject(tmpObj);
    }

    assert(0);
}


Proto cgBuildObject (Proto parseTree)
{

    int   i,j;
    Proto newObj;
    Proto array;
    Proto arrayName;
    Proto tmpObj;
    Proto annotation;
    int count;
    
    
    Proto proto = parseTree;

    
    newObj = objectNew(0);
    
    i = objectGetSlot(proto, stringToAtom("name"), &arrayName);
    assert(i);
    assert((Proto) stringToAtom("Object") == arrayName);

    // READ THIS
    // The situation The Proto might have two slots: 0 - SlotList, 1 - CodeBlock
    // or it might have one slot: 0 - CodeBlock
    // OR it might have NO slots ( a simple, empty proto )
    // The following code will handle either

    // Get the slot list and other optional parts
    count = arrayCount(proto);

    // If it is a simple empty proto, then return it here
    if (count == 0) {
	return newObj;
    }
	
    i = arrayGet(proto, 0, &array);
    assert(i);
    i = objectGetSlot(array, stringToAtom("name"), &tmpObj);
    assert(i);

    if ((Proto) stringToAtom("SlotList") == tmpObj) {
	j = arrayCount(array);
	
	// enumerate through the proto building it's slots
	for (i = 0; i < j; i++) {
	    arrayGet(array, i, &tmpObj);
            cgBuildSlot(tmpObj, newObj);
	}
	
	// Prep the next If statement
	if (1 == count) {
	    // There was only one slot
	    return newObj;
	}
	
	i = arrayGet(proto, 1, &array);
	assert(i);
	i = objectGetSlot(array, stringToAtom("name"), &tmpObj);
	assert(i);
    }

    // If an object contains a codeblock, then the object as a whole
    // is a code object. The cgBuildCodeBlock will handle either a method
    // or a block (since they are essentially the same)
    if ((Proto) stringToAtom("CodeBlock") == tmpObj) {
	assert(objectGetSlot(proto, stringToAtom("_annotate"), &annotation));
	tmpObj = cgBuildCodeBlock(array, annotation);
	objectSetSlot(newObj, (Atom) stringToAtom("_assemble"), PROTO_ACTION_GET, (uint32)tmpObj);
    }

    return newObj;
}


void cgBuildSlot (Proto parseTree, Proto newObj)
{

    int i;
    Proto tmp;
    Proto tmp2;
    Proto newValue;
    Proto slotName;
    Proto annotate;
    
    
    Proto proto = parseTree;

    
    arrayGet(proto, 0, &tmp);
    

    if ( (Proto) stringToAtom("Identifier-Slot") == tmp) {
	arrayGet(proto, 1, &tmp);
	objectSetSlot(newObj, (Atom) tmp, PROTO_ACTION_GET, (uint32) nil);
	return;
    }
    
    if ( (Proto) stringToAtom("Data-Get-Slot") == tmp) {
	arrayGet(proto, 1, &tmp);
	arrayGet(proto, 2, &tmp2);
	// The array is receiver
	newValue = cgBuildReceiver(tmp2);
	objectSetSlot(newObj, (Atom) tmp, PROTO_ACTION_GET, (uint32) newValue);
	return;
    }
    if ( (Proto) stringToAtom("Data-SetGet-Slot") == tmp) {
	arrayGet(proto, 1, &tmp);
	arrayGet(proto, 2, &tmp2);
	// The array is receiver
	newValue = cgBuildReceiver(tmp2);
	objectEasySetSlot(newObj, (Atom) tmp, (uint32) newValue);
	return;
    }
    if ( (Proto) stringToAtom("Arg-Slot") == tmp) {
	arrayGet(proto, 1, &slotName);
	arrayGet(proto, 2, &tmp2);
	// Get the order in the frame
	i = atoi(atomToString((Atom) tmp2));
	tmp2 = objectNewInteger(i);
	objectSetSlot(newObj, (Atom) slotName, PROTO_ACTION_GETFRAME, (uint32) tmp2);
	return;
    }

    if ( (Proto) stringToAtom("Local-SetGet-Slot") == tmp) {
	arrayGet(proto, 1, &slotName);
	arrayGet(proto, 2, &tmp2);
	// Get the order in the frame
	i = atoi(atomToString((Atom) tmp2));
	tmp2 = objectNewInteger(i);
	objectEasySetFrameSlot(newObj, (Atom) slotName, (uint32) tmp2);
	return;
    }

    if ( (Proto) stringToAtom("Local-Get-Slot") == tmp) {
	arrayGet(proto, 1, &slotName);
	arrayGet(proto, 2, &tmp2);
	// Get the order in the frame
	i = atoi(atomToString((Atom) tmp2));
	tmp2 = objectNewInteger(i);
	objectSetSlot(newObj, (Atom) slotName, PROTO_ACTION_GETFRAME, (uint32) tmp2);
	return;
    }

    if ( (Proto) stringToAtom("Unary-Slot") == tmp) {
	arrayGet(proto, 1, &slotName);
	// Get the 'array' holding the object
	arrayGet(proto, 2, &tmp2);
	// One more level down the 'array' holding the object
	arrayGet(tmp2, 1, &tmp2);
	newValue = cgBuildObject(tmp2);
	objectSetSlot(newObj, (Atom) slotName, PROTO_ACTION_GET, (uint32) newValue);
	return;
    }
    if ( (Proto) stringToAtom("Binary-Slot") == tmp) {
	// Get the 'array' holding the object
	arrayGet(proto, 1, &tmp2);
	arrayGet(tmp2, 0, &slotName);
	arrayGet(tmp2, 3, &tmp);
	newValue = cgBuildObject(tmp);
	objectSetSlot(newObj, (Atom) slotName, PROTO_ACTION_GET, (uint32) newValue);
	return;
    }
    
    if ( (Proto) stringToAtom("Keyword-Slot") == tmp) {
	// Get the 'array' holding the object
	arrayGet(proto, 1, &tmp2);
	i = arrayCount(tmp2);
	i--;
	assert(objectGetSlot(tmp2, stringToAtom("_annotate"), &annotate));
	assert(objectGetSlot(annotate, stringToAtom("slotName"), &slotName));
	arrayGet(tmp2, i, &tmp);
	newValue = cgBuildObject(tmp);
	objectSetSlot(newObj, (Atom) slotName, PROTO_ACTION_GET, (uint32) newValue);
	return;
    }

    // We shouldn't get here
    assert(0);
}

Proto cgBuildArray (Proto parseTree)
{
    Proto tmp;
    Proto tmpObj;
    int i;
    int j;
    Proto newArray;
    
    Proto proto = parseTree;


    newArray = createArray((Proto)stringToAtom("newArray"));
    
    j = arrayCount(proto);

    // enumerate through the proto building it's slots
    for (i = 0; i < j; i++) {
	arrayGet(proto, i, &tmpObj);
	tmp = cgBuildReceiver(tmpObj);
	addToArray(newArray, tmp);
    }
    
    return newArray;
}


Proto cgBuildReceiver (Proto parseTree)
{

    Proto tmp;
    int i;
    double f;

    Proto proto = parseTree;

    
    arrayGet(proto, 0, &tmp);
    
    if ( (Proto) stringToAtom("String") == tmp) {
	arrayGet(proto, 1, &tmp);
	return tmp;
    }
    if ( (Proto) stringToAtom("Object") == tmp) {
	arrayGet(proto, 1, &tmp);
	return cgBuildObject(tmp);
    }
    if ( (Proto) stringToAtom("Integer") == tmp) {
	arrayGet(proto, 1, &tmp);
	i = atoi(atomToString((Atom) tmp));
	return objectNewInteger(i);
    }
    if ( (Proto) stringToAtom("Float") == tmp) {
	arrayGet(proto, 1, &tmp);
	f = atof(atomToString((Atom) tmp));
	return objectNewFloat(f);
    }
    if ( (Proto) stringToAtom("Nil Expression") == tmp) {
    	return nil;
    }
    if ( (Proto) stringToAtom("self") == tmp) {
    	return nil;
    }
    if ( (Proto) stringToAtom("Array") == tmp) {
	arrayGet(proto, 1, &tmp);
	return cgBuildArray(tmp);
    }
    if ( (Proto) stringToAtom("Expression") == tmp) {
	arrayGet(proto, 1, &tmp);
	return cgBuildExpression(tmp);
    }

    // We shouldn't get here
    assert(0);
}

Proto cgBuildReceiverAssembly (Proto parseTree)
{

    int i;
    Proto tmp;
    Proto object;
    Proto assemble;
    Proto newObj;
    
    Proto proto = parseTree;

    
    objectGetSlot(proto, stringToAtom("name"), &tmp);
    if ( (Proto) stringToAtom("Receiver") == tmp) {
	object = cgBuildReceiver(proto);
    }
    else if ( (Proto) stringToAtom("Binary") == tmp) {
	object = cgBuildBinary(proto);
    }
    else if ( (Proto) stringToAtom("Unary") == tmp) {
	object = cgBuildUnary(proto);
    } else {
	object = nil;
    }
    

    i  = PROTO_REFTYPE(object);

    if (PROTO_REF_PROTO == i) {
	if (object != nil) {
	    objectGetSlot(object, stringToAtom("name"), &tmp);
	    if ( (Proto) stringToAtom("__assemble") == tmp) {
		return object;
	    }
	}
	
    }
	
    assemble = createArray( (Proto) stringToAtom("__assemble"));
    newObj = objectNew(0);
    
    if (object == nil) {
	objectSetSlot(newObj, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushSelf"));
	addToArray(assemble, newObj);
	return assemble;
    }
    
    objectSetSlot(newObj, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(newObj, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(newObj, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) object);
    addToArray(assemble, newObj);
    return assemble;
}



Proto cgBuildCodeBlock (Proto parseTree, Proto annotation)
{
    int i,j;
    Proto tmp;
    Proto tmp2;
    Proto assemble;
    Proto assemble2;
    Proto slotName;
    Proto localCount;
    Proto paramCount;
    Proto initialValues;
    Proto tmpValue;
    
        
    Proto proto = parseTree;
    
    
    assemble = createArray( (Proto) stringToAtom("__assemble"));

    i = objectGetSlot(annotation, stringToAtom("localCount"), &localCount);
    assert(i);
    
    i = objectGetSlot(annotation, stringToAtom("paramCount"), &paramCount);
    assert(i);
    
    i = objectGetSlot(annotation, stringToAtom("slotName"), &slotName);
    assert(i);
    
    i = objectGetSlot(annotation, stringToAtom("initialValues"), &initialValues);
    if (i == 0) {
	initialValues = nil;
    }
    

    // Slot Name
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("slotName"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) slotName);
    addToArray(assemble, tmp);
    
    // Setup Frame for Locals
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("setupFrame"));
//    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) localCount);
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(0));
    addToArray(assemble, tmp);
    

    // If we have initial values, then we initialize those slots
    if (initialValues != nil) {
	j = arrayCount(initialValues);
	for (i = 0; i < j; i++) {
	    arrayGet(initialValues, i, &tmpValue);
	    tmp = objectNew(0);
	    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) tmpValue);
	    addToArray(assemble, tmp);
	}
    }
    
    j = arrayCount(proto);
    
    // enumerate through the code block - an array of expressions
    i = 0;
    while (1) {
	arrayGet(proto, i, &tmp2);
	assemble2 = cgBuildExpression(tmp2);

	appendArrayToArray(assemble2, assemble);
	
	i++;
	

	if (i == j) {
	    tmp = objectNew(0);
	    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("restoreFrame"));
	    addToArray(assemble, tmp);
	    
	    tmp = objectNew(0);
	    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("return"));
	    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(3 + objectIntegerValue(paramCount)));
	    addToArray(assemble, tmp);
	    break;
	} else {
	    // Each expression's result is not used
	    tmp = objectNew(0);
	    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pop"));
	    addToArray(assemble, tmp);
	}
    }
    
    return assemble;
}

Proto cgBuildExpression (Proto parseTree)
{
    int i;
    Proto tmp;
    Proto assemble;
    Proto array;
    
        
    Proto proto = parseTree;
    
    
    i = arrayGet(proto, 0, &array);
    assert(i);

    i = objectGetSlot(array, stringToAtom("name"), &tmp);
    assert(i);

    if ( (Proto) stringToAtom("Unary") == tmp) {
	assemble = cgBuildUnary(array);
	return assemble;
    }
    
    if ( (Proto) stringToAtom("Binary") == tmp) {
	assemble = cgBuildBinary(array);
	return assemble;
    }

    if ( (Proto) stringToAtom("Message") == tmp) {
	assemble = cgBuildMessage(array);
	return assemble;
    }
    
    // Shouldn't get here
    assert(0);
}




Proto cgBuildUnary (Proto parseTree)
{
    int i,j;
    Proto tmp;
    Proto operand;
    Proto target;
    Proto assemble;
    
    Proto proto = parseTree;
    
    j = 0;
    i = arrayCount(proto);

    assemble = createArray( (Proto) stringToAtom("__assemble"));
    
    while (1) {
	
	if (0 == j) {
	    // 0 - get the first object handled by code gen
	    arrayGet(proto, 0, &tmp);
	    target = cgBuildReceiverAssembly(tmp);
	    j++;
	    j++;
	    
	    arrayGet(proto, 1, &operand);
	    
	    tmp = objectNew(0);
	    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) operand);
	    addToArray(assemble, tmp);
	    appendArrayToArray(target, assemble);
	} else {
	    // Access the array by the keys in order
	    arrayGet(proto, j, &operand);
	    
	    tmp = objectNew(0);
	    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) operand);
	    addToArray(assemble, tmp);
	    
	    tmp = objectNew(0);
	    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("swap"));
	    addToArray(assemble, tmp);
	    j++;
	}
	
	// Push the argument count
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(2));
	addToArray(assemble, tmp);
	
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("call"));
	addToArray(assemble, tmp);
	
	if (j == i) {
	    break;
	}
    }
    
    return assemble;
}






Proto cgBuildBinary (Proto parseTree)
{
    int i,j;
    Proto tmp;
    Proto operand;
    Proto operator;
    Proto target;
    Proto assemble;
    
    Proto proto = parseTree;
    
    i = arrayCount(proto);
    j = 0;
    
    assemble = createArray( (Proto) stringToAtom("__assemble"));
    
    // Need to do leftToRight, push right, push left, push message

    // Work backwards
    // An expression like 3 + 4 - 1 is (3 + 4) - 1
    // However since this is a stack machine
    // the code on the stack will be   1-4+3CC  
    // (C represents Call), otherwise there would be lots of swaps on
    // the stack
    // Problem: (3 + 4) - 5 - 1
    while (1) {
	
	if (j == 0) {
	    arrayGet(proto, j, &tmp);
	    target = cgBuildReceiverAssembly(tmp);
	    j++;
	    // Push the target
	    appendArrayToArray(target, assemble);
	}
	
	
	arrayGet(proto, j, &operator);
	j++;

	arrayGet(proto, j, &tmp);
	operand = cgBuildReceiverAssembly(tmp);
	j++;
	// Push the operand
	appendArrayToArray(operand, assemble);
	// swap
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("swap"));
	addToArray(assemble, tmp);
	

	// Push the mesg/operator
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) operator);
	addToArray(assemble, tmp);
	// swap
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("swap"));
	addToArray(assemble, tmp);

	
	// Push the argument count
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
	objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
	objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(3));
	addToArray(assemble, tmp);
	
	
	tmp = objectNew(0);
	objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("call"));
	addToArray(assemble, tmp);
	
	if (j == i) {
	    break;
	}
    }
    
    return assemble;

}



Proto cgBuildMessage(Proto parseTree)
{
    int i,j;
    Proto tmp;
    Proto messageName;
    Proto operand;
    Proto target;
    Proto assemble;
    Proto operator;
    
    
    Proto proto = parseTree;
    
    
    // Get all of the arguments prepared
    // Then setup the call to the method
    
    // Just like Binary, we work backwards since that is how
    // arguments are pushed on the stack
    
    // Essentially, go through each operand in reverse and add
    // them as operands
    // Then also push the method name (after it has been 
    // concatenated)
    // push self (or target object)
    // make call
    
    i = arrayCount(proto);
    j = i - 1;
    
    
    assemble = createArray( (Proto) stringToAtom("__assemble"));
    messageName = createArray( (Proto) stringToAtom("MessageName"));
    
    // Go in reverse and push the arguments while collecting the 
    // Messages
    while (1) {
	
	arrayGet(proto, j, &tmp);
	operand = cgBuildReceiverAssembly(tmp);
	j--;
	appendArrayToArray(operand, assemble);
	
	arrayGet(proto, j, &operator);
	addToArray(messageName, operator);
	j--;
	
	if (0 == j) {
	    break;
	}
    }
    
    
    // Ok it is now time to generate the message
    
    // Go through the messageName forward and write that string
    // as the selector
    // Go through the args in reverse and push the 
    i = arrayCount(messageName);
    
    j = i - 1;
    stringBuffer[0] = 0;
    
    while (1) {
	arrayGet(messageName, j, &operand);
	
	strcat(stringBuffer, atomToString((Atom) operand));
	
	j--;
	
	if (j < 0) {
	    break;
	}
    }
    
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) stringToAtom(stringBuffer));
    addToArray(assemble, tmp);
    
    // Finally, push the target
    arrayGet(proto, 0, &tmp);
    target = cgBuildReceiverAssembly(tmp);
    appendArrayToArray(target, assemble);

    // Push the argument count
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("pushimmediate"));
    objectSetSlot(tmp, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("object"));
    objectSetSlot(tmp, stringToAtom("operand"), PROTO_ACTION_GET, (uint32) objectNewInteger(i+2));
    addToArray(assemble, tmp);
    
	
    tmp = objectNew(0);
    objectSetSlot(tmp, stringToAtom("op"), PROTO_ACTION_GET, (uint32) stringToAtom("call"));
    addToArray(assemble, tmp);
    
    return assemble;
}

