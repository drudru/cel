
//
// runtime.c
//
// <see corresponding .h file>
// 
//
// $Id: runtime.c,v 1.48 2002/02/22 22:23:24 dru Exp $
//
//


#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>
#include "config.h"

#include "svm32.h"
#include "cel.h"
#include "runtime.h"
#include "atom.h"
#include "alloc.h"
#include "assemble.h"
#include "array.h"
#include "compiler.h"
#include "while.h"
#include "sd2cel.h"
#include "cel2sd.h"




// INTERNAL G L O B A L S   &   H O O K S

char * celActionTypes[] = { "Get", "Set",
			     "GetStack", "SetStack",
			     "Passthru",
			     "Code",
			     "Intrinsic", "Seven"};


char * SearchPaths[] = { "", 
			 ".",
			 "./kits",
			 "../kits",
			 "/usr/local/cel/kits",
			 ""};

int PROTO_FillFactor = 8;

Proto Lobby;
Proto DefaultBehavior;
Proto Globals;
Proto Traits;
Proto Clonable;
Proto ObjectClass;
Proto StringParent;
Proto ArrayParent;
Proto FloatParent;
Proto OddBalls;
Proto Kit;

Proto IntegerClass;
Proto AtomClass;
Proto Capsule;
Proto GC;
Proto SimpleDataFactory;



// The default debugging info handler
int (*printString)(char * str);        /* Output Handler    */
int (*ioOut)(char * str, int count);   /* IO Output Handler */
int (*addPrintSpaces)(int32 spaces);   /* Output Handler    */
int printIndent = 0;

int runtimeActivateTrace;
#ifdef unix
struct timeval lasttv;
#endif


VM32Cpu * Cpu;
// Preset Atoms - we initialize these since they are used
// so often. PRESET
Atom ATOMparent;
Atom ATOMblockParent;
Atom ATOM_code_;
//Atom DebugName;
Atom ATOMString;
Atom ATOM_string;
Atom ATOMsize;
Atom ATOMcapacity;
Atom ATOMname;
Atom ATOM_capacity;
Atom ATOM_array;
Atom ATOM_float;
Atom ATOM_assemble;
Atom ATOMop;
Atom ATOMnop;
Atom ATOMbreak;
Atom ATOMslotName;
Atom ATOMpop;
Atom ATOMswap;
Atom ATOMcall;
Atom ATOMsetupFrame;
Atom ATOMrestoreFrame;
Atom ATOMpushSelf;
Atom ATOMreturn;
Atom ATOMpushimmediate;
Atom ATOMargCountCheck;
Atom ATOMbranch;
Atom ATOMbranchIfTrue;
Atom ATOMbranchIfFalse;
Atom ATOMoperand;
Atom ATOM_Name_;
Atom ATOMtype;
Atom ATOMobject;
Atom ATOMself;
Atom ATOMactivateSlot;
Atom ATOMdate;
Atom ATOM_datetime;



// Special Objects
Proto TrueObj;
Proto FalseObj;

Proto WhileTrueObj;
Proto WhileFalseObj;


int debugDump  = 0;
int debugTrace = 0;
int noPrint    = 0;


// This routine will do simple string output to io
// print indents upon any newlines in the string.
int defaultIoOut(char * str, int count)
{
    if (noPrint) {
	return 0;
    }
    
    write (1, str, count);
    return 0;
}


// This routine will print a string, but it will properly
// print indents upon any newlines in the string.
int defaultPrintString(char * str)
{
    char * start;
    char * end;
    
    int i;


    start = end = str;

    while (1) {
	if (*end == '\n') {

	    // If there was stuff to print before this, then print it
	    if (start != end) {
		ioOut(start, (end - start));
	    }

	    // Print spaces (the indent)
	    //// A highly inefficient indenter...
	    ioOut("\n", 1);
	    for (i = 0; i < printIndent; i++) {
		ioOut(" ", 1);
	    }
	    end++;
	    start = end;
	}

	if (*end == 0) {
	    // If there was stuff to print before this, then print it
	    if (start != end) {
		ioOut (start, (end - start));
	    }
	    break;
	}
	
	end++;
    }
    
    return 0;
}

int defaultAddPrintSpaces(int32 spaces)
{
    printIndent += spaces;

    if (printIndent < 0) {
	printIndent = 0;
	defaultPrintString("Someone botched the number of spaces to print.\n");
    }
    
    return 0;
}

// This routine sets up the Atom and the Lobby and the internal intrinsics
int celInit (VM32Cpu * cpu) 
{
    Proto tmpObj;
    Proto tmpObj2;
    
    // Initialize the garbage collection system
    gcInit();
    
    GCOFF();
    

    // Architecture specific initialization
#ifdef WIN32
    _fmode = _O_BINARY;
#endif
    
    AtomInit();
    printString = defaultPrintString;
    addPrintSpaces = defaultAddPrintSpaces;
    ioOut       = defaultIoOut;
    
    Cpu = cpu;

    // Initialize some common atoms PRESET
    ATOMparent        = stringToAtom("parent");
    ATOMblockParent   = stringToAtom("blockParent");
    ATOM_code_        = stringToAtom("_code_");
//    ATOMDebugName     = stringToAtom("_debugName_");
    ATOMString    = stringToAtom("String");
    ATOM_string   = stringToAtom("_string");
    ATOMsize      = stringToAtom("size");
    ATOMcapacity  = stringToAtom("capacity");
    ATOMname      = stringToAtom("name");
    ATOM_capacity = stringToAtom("_capacity");
    ATOM_array    = stringToAtom("_array");
    ATOM_float    = stringToAtom("_float");
    ATOM_assemble = stringToAtom("_assemble");
    ATOMop        = stringToAtom("op");
    ATOMnop       = stringToAtom("nop");
    ATOMbreak     = stringToAtom("break");
    ATOMslotName  = stringToAtom("slotName");
    ATOMpop       = stringToAtom("pop");
    ATOMswap       = stringToAtom("swap");
    ATOMcall       = stringToAtom("call");
    ATOMsetupFrame       = stringToAtom("setupFrame");
    ATOMrestoreFrame       = stringToAtom("restoreFrame");
    ATOMpushSelf       = stringToAtom("pushSelf");
    ATOMreturn       = stringToAtom("return");
    ATOMpushimmediate       = stringToAtom("pushimmediate");
    ATOMargCountCheck       = stringToAtom("argCountCheck");
    ATOMbranch       = stringToAtom("branch");
    ATOMbranchIfTrue       = stringToAtom("branchIfTrue");
    ATOMbranchIfFalse       = stringToAtom("branchIfFalse");
    ATOMoperand       = stringToAtom("operand");
    ATOM_Name_       = stringToAtom("_Name_");
    ATOMtype       = stringToAtom("type");
    ATOMobject       = stringToAtom("object");
    ATOMself       = stringToAtom("self");
    ATOMactivateSlot       = stringToAtom("activateSlot");
    ATOMdate            = stringToAtom("date");
    ATOM_datetime       = stringToAtom("_datetime");
    

    // ** Special ** Needs to be initialized before all others since arrays
    // depend on this
    ArrayParent = objectNew(0);

    
    runtimeActivateTrace = 0;

    // Create the True and False objects
    tmpObj  = eval("{| true  <- {| type <- 'Boolean'. asString <- 'true'. ifTrue: a ifFalse: b <+ { a value. }. and: a <+ {  a value. }. ifTrue: a <+ { a value. }. ifFalse: a <+ { false.}. print <+ { '\\'True\\'' print. }. |}. false <- {| type <- 'Boolean'. asString <- 'false'. ifTrue: a ifFalse: b <+ { b value. }.  and: a <+ { false. }. ifTrue: a <+ { false. }. ifFalse: a <+ { a value.}. print <+ { '\\'False\\'' print. }. |} |}", 0, 0);
    
    assembleAll(tmpObj);
    objectGetSlot(tmpObj, stringToAtom("true"),  &TrueObj);
    objectGetSlot(tmpObj, stringToAtom("false"), &FalseObj);
    Cpu->trueObj  = (uint32) TrueObj;
    Cpu->falseObj = (uint32) FalseObj;

    // Set up the root set, create the lobby
    Lobby = objectNew(0);

    // Create and setup DefaultBehavior
    DefaultBehavior = objectNew(0);
    objectSetSlot(Lobby, stringToAtom("DefaultBehavior"), PROTO_ACTION_GET, (uint32) DefaultBehavior);
    objectSetSlot(DefaultBehavior, stringToAtom("setSlot:with:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicSetSlotWith);
    objectSetSlot(DefaultBehavior, stringToAtom("slotNames"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicSlotNames);
    objectSetSlot(DefaultBehavior, stringToAtom("hasSlot:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicHasSlot);
    objectSetSlot(DefaultBehavior, stringToAtom("getSlot:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicGetSlot);
    objectSetSlot(DefaultBehavior, stringToAtom("removeSlot:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicRemoveSlot);
    // *NOTE* Same intrinsic for all of the performs
    objectSetSlot(DefaultBehavior, stringToAtom("perform:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicPerform);
    objectSetSlot(DefaultBehavior, stringToAtom("perform:with:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicPerform);
    objectSetSlot(DefaultBehavior, stringToAtom("perform:with:with:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicPerform);

    objectSetSlot(DefaultBehavior, stringToAtom("objectDump"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicObjectDump);
    objectSetSlot(DefaultBehavior, stringToAtom("print"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicObjectShallowDump);
    objectSetSlot(DefaultBehavior, stringToAtom("objectShallowDump"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicObjectShallowDump);
    objectSetSlot(DefaultBehavior, stringToAtom("traceOff"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicTrace);
    objectSetSlot(DefaultBehavior, stringToAtom("traceOn"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicTrace);
    objectSetSlot(DefaultBehavior, stringToAtom("isNil"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicObjectIsNil);
    objectSetSlot(DefaultBehavior, stringToAtom("className"), PROTO_ACTION_GET, (uint32) stringToAtom("<DefaultBehavior>"));
    objectSetSlot(DefaultBehavior, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("Proto"));
    objectSetSlot(DefaultBehavior, stringToAtom("value"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicValue);
    objectSetSlot(DefaultBehavior, stringToAtom("value:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicValue);

    // Build the special whileTrue:, whileFalse: slots
    objectSetSlot(DefaultBehavior, stringToAtom("whileTrue:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicWhileTrue);
    objectSetSlot(DefaultBehavior, stringToAtom("whileFalse:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicWhileFalse);
    WhileTrueObj = objectResolveForwarders( whileObj(1) );
    assemble(WhileTrueObj, nil, 1);
    objectDeleteSlot(WhileTrueObj, ATOMparent);
    
    WhileFalseObj = objectResolveForwarders( whileObj(0) );
    assemble(WhileFalseObj, nil, 1);
    objectDeleteSlot(WhileFalseObj, ATOMparent);
    // NOTE - these are referenced in the oddballs object below
    // SINCE they should never change after this point, they
    // are immortal and all references will never point to
    // a forwarder
    
    
    // Setup the Traits
    Traits = objectNew(0);
    objectSetSlot(Lobby, stringToAtom("Traits"), PROTO_ACTION_GET, (uint32) Traits);
    // ..Clonable
    Clonable = objectNew(0);
    objectSetSlot(Traits, stringToAtom("Clonable"), PROTO_ACTION_GET, (uint32) Clonable);

    ObjectClass = objectNew(0);
    objectSetSlot(Clonable, stringToAtom("ObjectClass"), PROTO_ACTION_GET, (uint32) ObjectClass);
    objectSetSlot(ObjectClass, stringToAtom("className"), PROTO_ACTION_GET, (uint32) stringToAtom("Object"));
    objectSetSlot(ObjectClass, stringToAtom("clone"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicClone);

    StringParent = objectNew(0);
    objectSetSlot(Clonable, stringToAtom("StringParent"), PROTO_ACTION_GET, (uint32) StringParent);
    objectSetSlot(StringParent, ATOMparent, PROTO_ACTION_GET, (uint32) ObjectClass);
    objectSetSlot(StringParent, stringToAtom("className"), PROTO_ACTION_GET, (uint32) stringToAtom("String"));
    objectSetSlot(StringParent, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("String"));
    objectSetSlot(StringParent, stringToAtom("print"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringPrint);
    objectSetSlot(StringParent, stringToAtom("="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringEqual);
    objectSetSlot(StringParent, stringToAtom(">"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringGreater);
    objectSetSlot(StringParent, stringToAtom(">="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringGreaterEqual);
    objectSetSlot(StringParent, stringToAtom("<"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringLess);
    objectSetSlot(StringParent, stringToAtom("<="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringLessEqual);
    objectSetSlot(StringParent, stringToAtom("+"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAdd);
    objectSetSlot(StringParent, stringToAtom("at:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAt);
    objectSetSlot(StringParent, stringToAtom("insert:at:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringInsertAt);
    objectSetSlot(StringParent, stringToAtom("removeAt:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringRemoveAt);
    objectSetSlot(StringParent, stringToAtom("removeChar:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringRemoveChar);
    objectSetSlot(StringParent, stringToAtom("append:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAppend);
    objectSetSlot(StringParent, stringToAtom("reverse"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringReverse);
    objectSetSlot(StringParent, stringToAtom("lowerCase"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringLowercase);
    objectSetSlot(StringParent, stringToAtom("asString"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicSelf);
    objectSetSlot(StringParent, stringToAtom("asAtom"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAsAtom);
    objectSetSlot(StringParent, stringToAtom("setCapacity:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSetCapacity);
    objectSetSlot(StringParent, stringToAtom("splitOn:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSplit);
    objectSetSlot(StringParent, stringToAtom("splitOn:atMostCount:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSplit);
    objectSetSlot(StringParent, stringToAtom("splitCSV"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSplitCSV);
    objectSetSlot(StringParent, stringToAtom("copyFrom:To:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSlice);
    objectSetSlot(StringParent, stringToAtom("asInteger"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAsInteger);
    objectSetSlot(StringParent, stringToAtom("asIntegerScaled:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAsIntegerScaled);
    objectSetSlot(StringParent, stringToAtom("asFloat"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAsFloat);
    objectSetSlot(StringParent, stringToAtom("findLast:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringFindLast);
    objectSetSlot(StringParent, stringToAtom("urlUnEscape"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringUrlUnEscape);
    objectSetSlot(StringParent, stringToAtom("urlEscape"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringUrlEscape);

    FloatParent = objectNew(0);
    objectSetSlot(Clonable, stringToAtom("FloatParent"), PROTO_ACTION_GET, (uint32) FloatParent);
    objectSetSlot(FloatParent, ATOMparent, PROTO_ACTION_GET, (uint32) ObjectClass);
    objectSetSlot(FloatParent, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("Float"));
    objectSetSlot(FloatParent, stringToAtom("copy"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatCopy);
    objectSetSlot(FloatParent, stringToAtom("+"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatAdd);
    objectSetSlot(FloatParent, stringToAtom("-"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatSubtract);
    objectSetSlot(FloatParent, stringToAtom("*"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatMultiply);
    objectSetSlot(FloatParent, stringToAtom("/"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatDivide);
    objectSetSlot(FloatParent, stringToAtom("abs"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatAbsolute);
    objectSetSlot(FloatParent, stringToAtom("@"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicPointCreate);
    objectSetSlot(FloatParent, stringToAtom("="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatEqual);
    objectSetSlot(FloatParent, stringToAtom("!="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatNotEqual);
    objectSetSlot(FloatParent, stringToAtom(">"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatGreater);
    objectSetSlot(FloatParent, stringToAtom(">="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatGreaterEqual);
    objectSetSlot(FloatParent, stringToAtom("<"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatLess);
    objectSetSlot(FloatParent, stringToAtom("<="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatLessEqual);
    objectSetSlot(FloatParent, stringToAtom("print"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatPrint);
    objectSetSlot(FloatParent, stringToAtom("asString"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatAsString);
    objectSetSlot(FloatParent, stringToAtom("asStringWithFormat:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatAsString);
    objectSetSlot(FloatParent, stringToAtom("asInteger"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatAsInteger);
    objectSetSlot(FloatParent, stringToAtom("asIntegerScaled:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFloatAsIntegerScaled);



    // Test if Array compiled file exists
    // if not, that's ok
    // if so, import it
    tmpObj = intrinsicLoadModule(stringToAtom("Array"));
    if (tmpObj != nil) {
	// Ok find the root object
	objectGetSlot(tmpObj, stringToAtom("Root"), &tmpObj2);
	assembleAll(tmpObj2);

	// Now set the original proto as a forwarder
	// Need to make this atomic (maybe do it as a long long store?)
	ArrayParent->data[0] = (uint32) tmpObj2;      // have the count be the pointer to the data
	ArrayParent->count |= PROTO_COUNT_FORWARDER; // mark the object as a forwarder

	ArrayParent = tmpObj2;
    }
    
    // ** NOTE ** Array parent initialized above since it is needed before this is used

    objectSetSlot(Clonable, stringToAtom("ArrayParent"), PROTO_ACTION_GET, (uint32) ArrayParent);
    objectSetSlot(ArrayParent, ATOMparent, PROTO_ACTION_GET, (uint32) ObjectClass);
    objectSetSlot(ArrayParent, stringToAtom("className"), PROTO_ACTION_GET, (uint32) stringToAtom("Array"));
    objectSetSlot(ArrayParent, stringToAtom("new"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicArrayNew);
    objectSetSlot(ArrayParent, stringToAtom("new:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicArrayNewSize);
    objectSetSlot(ArrayParent, stringToAtom("at:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicArrayAt);
    objectSetSlot(ArrayParent, stringToAtom("at:otherwiseUse:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicArrayAtOtherwiseUse);
    objectSetSlot(ArrayParent, stringToAtom("at:Put:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicArrayAtPut);
    objectSetSlot(ArrayParent, stringToAtom("insert:at:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicArrayInsertAt);
    objectSetSlot(ArrayParent, stringToAtom("removeAt:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicArrayRemoveAt);
    objectSetSlot(ArrayParent, stringToAtom("append:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicArrayAppend);



    // ..OddBalls
    OddBalls = objectNew(0);
    objectSetSlot(Traits, stringToAtom("OddBalls"), PROTO_ACTION_GET, (uint32) OddBalls);


    // The WhileTrue and WhileFalse Objects
    objectSetSlot(OddBalls, stringToAtom("WhileTrueObj"), PROTO_ACTION_GET, (uint32) WhileTrueObj);
    objectSetSlot(OddBalls, stringToAtom("WhileFalseObj"), PROTO_ACTION_GET, (uint32) WhileFalseObj);

    
    IntegerClass = objectNew(0);
    objectSetSlot(OddBalls, stringToAtom("IntegerClass"), PROTO_ACTION_GET, (uint32) IntegerClass);
    objectSetSlot(IntegerClass, stringToAtom("className"), PROTO_ACTION_GET, (uint32) stringToAtom("Integer"));
    objectSetSlot(IntegerClass, stringToAtom("+"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerAdd);
    objectSetSlot(IntegerClass, stringToAtom("-"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerSubtract);
    objectSetSlot(IntegerClass, stringToAtom("*"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerMultiply);
    objectSetSlot(IntegerClass, stringToAtom("/"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerDivide);
    objectSetSlot(IntegerClass, stringToAtom("%"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerModulo);
    objectSetSlot(IntegerClass, stringToAtom("abs"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerAbsolute);
    objectSetSlot(IntegerClass, stringToAtom("@"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicPointCreate);
    objectSetSlot(IntegerClass, stringToAtom("="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerEqual);
    objectSetSlot(IntegerClass, stringToAtom("!="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerNotEqual);
    objectSetSlot(IntegerClass, stringToAtom(">"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerGreater);
    objectSetSlot(IntegerClass, stringToAtom(">="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerGreaterEqual);
    objectSetSlot(IntegerClass, stringToAtom("<"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerLess);
    objectSetSlot(IntegerClass, stringToAtom("<="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerLessEqual);
    objectSetSlot(IntegerClass, stringToAtom("print"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerPrint);
    objectSetSlot(IntegerClass, stringToAtom("asFloat"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerAsFloat);
    objectSetSlot(IntegerClass, stringToAtom("asString"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerAsString);
    objectSetSlot(IntegerClass, stringToAtom("asStringWithFormat:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicIntegerAsString);
    
    
    AtomClass = objectNew(0);
    objectSetSlot(OddBalls, stringToAtom("AtomClass"), PROTO_ACTION_GET, (uint32) AtomClass);
    objectSetSlot(AtomClass, stringToAtom("className"), PROTO_ACTION_GET, (uint32) stringToAtom("Atom"));
    objectSetSlot(AtomClass, stringToAtom("type"), PROTO_ACTION_GET, (uint32) stringToAtom("Atom"));
    objectSetSlot(AtomClass, stringToAtom("print"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomPrint);
    objectSetSlot(AtomClass, ATOMsize, PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomSize);
    objectSetSlot(AtomClass, stringToAtom("asString"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomAsString);
    objectSetSlot(AtomClass, stringToAtom("="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomEqual);
    objectSetSlot(AtomClass, stringToAtom("!="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomNotEqual);
    objectSetSlot(AtomClass, stringToAtom(">"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomGreater);
    objectSetSlot(AtomClass, stringToAtom(">="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomGreaterEqual);
    objectSetSlot(AtomClass, stringToAtom("<"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomLess);
    objectSetSlot(AtomClass, stringToAtom("<="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicAtomLessEqual);
    // Note: these are methods that reuse the intrinsic for the String objects
    objectSetSlot(AtomClass, stringToAtom("+"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAdd);
    objectSetSlot(AtomClass, stringToAtom("splitOn:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSplit);
    objectSetSlot(AtomClass, stringToAtom("splitOn:atMostCount:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSplit);
    objectSetSlot(AtomClass, stringToAtom("splitCSV"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSplitCSV);
    objectSetSlot(AtomClass, stringToAtom("copyFrom:To:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringSlice);
    objectSetSlot(AtomClass, stringToAtom("asInteger"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAsInteger);
    objectSetSlot(AtomClass, stringToAtom("asIntegerScaled:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAsIntegerScaled);
    objectSetSlot(AtomClass, stringToAtom("asFloat"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringAsFloat);
    objectSetSlot(AtomClass, stringToAtom("findLast:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicStringFindLast);


    // Setup the Global
    Globals = objectNew(0);
    objectSetSlot(Lobby, stringToAtom("Globals"), PROTO_ACTION_GET, (uint32) Globals);
    objectSetSlot(Lobby, ATOMparent, PROTO_ACTION_GET, (uint32) Globals);

    // Object
    tmpObj = objectNew(0);
    objectSetSlot(Globals, stringToAtom("Object"), PROTO_ACTION_GET, (uint32) tmpObj);
    objectSetSlot(tmpObj, ATOMparent, PROTO_ACTION_GET, (uint32) ObjectClass);

    // String
    tmpObj = objectNew(0);
    objectSetSlot(Globals, stringToAtom("String"), PROTO_ACTION_GET, (uint32) tmpObj);
    objectSetSlot(tmpObj, ATOMparent, PROTO_ACTION_GET, (uint32) StringParent);
    objectSetSlot(tmpObj, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(0));
    objectSetSlot(tmpObj, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(0));

    // Float
    tmpObj = objectNewFloat(0.0);
    objectSetSlot(Globals, stringToAtom("Float"), PROTO_ACTION_GET, (uint32) tmpObj);

    // Array
    tmpObj = createArray(nil);
    objectSetSlot(Globals, stringToAtom("Array"), PROTO_ACTION_GET, (uint32) tmpObj);
    objectSetSlot(tmpObj, ATOMparent, PROTO_ACTION_GET, (uint32) ArrayParent);

    // True/False
    objectSetSlot(Globals,  stringToAtom("true"),       PROTO_ACTION_GET,       (uint32) TrueObj);
    objectSetSlot(Globals,  stringToAtom("false"),      PROTO_ACTION_GET,       (uint32) FalseObj);
    objectSetSlot(TrueObj,  stringToAtom("="),  PROTO_ACTION_INTRINSIC, (uint32) intrinsicTrueEqual);
    objectSetSlot(TrueObj,  stringToAtom("!="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicTrueNotEqual);
    objectSetSlot(FalseObj, stringToAtom("="),  PROTO_ACTION_INTRINSIC, (uint32) intrinsicFalseEqual);
    objectSetSlot(FalseObj, stringToAtom("!="), PROTO_ACTION_INTRINSIC, (uint32) intrinsicFalseNotEqual);
    
    // Kit
    tmpObj = objectNew(0);
    Kit = tmpObj;
    objectSetSlot(Globals, stringToAtom("Kit"), PROTO_ACTION_GET, (uint32) tmpObj);
    objectSetSlot(Kit, stringToAtom("import:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicKitImport);
    objectSetSlot(Kit, stringToAtom("importLib:"), PROTO_ACTION_INTRINSIC, (uint32) intrinsicKitImportLib);
    // This is where the kit keeps track of what was imported
    tmpObj = objectNew(0);
    objectSetSlot(Kit, stringToAtom("imported"), PROTO_ACTION_GET, (uint32) tmpObj);
    tmpObj = objectNew(0);
    objectSetSlot(Kit, stringToAtom("libraries"), PROTO_ACTION_GET, (uint32) tmpObj);
    

    // SimpleDataFactory
    SimpleDataFactory = tmpObj = objectNew(0);
    objectSetSlot(Globals, stringToAtom("SimpleDataFactory"), PROTO_ACTION_GET, (uint32) tmpObj);
    objectSetSlot(tmpObj, stringToAtom("getFromFile:"),   PROTO_ACTION_INTRINSIC, (uint32) intrinsicSDGetFromFile);
    objectSetSlot(tmpObj, stringToAtom("saveToFile:"),   PROTO_ACTION_INTRINSIC, (uint32) intrinsicSDSaveToFile);
    objectSetSlot(tmpObj, stringToAtom("fromString:"),   PROTO_ACTION_INTRINSIC, (uint32) intrinsicSDfromString);
    objectSetSlot(tmpObj, stringToAtom("asString:"),   PROTO_ACTION_INTRINSIC, (uint32) intrinsicSDasString);
    objectSetSlot(tmpObj, ATOMparent, PROTO_ACTION_GET, (uint32) ObjectClass);


    // Capsule - where things get put by the lower systems for retrieval by
    // various kits (Environment variables, command line arguments, etc.)
    Capsule = objectNew(0);
    objectSetSlot(Globals, stringToAtom("Capsule"), PROTO_ACTION_GET, (uint32) Capsule);

    GC = objectNew(0);
    objectSetSlot(Globals, stringToAtom("GC"), PROTO_ACTION_GET, (uint32) GC);
    objectSetSlot(GC, stringToAtom("run"),  PROTO_ACTION_INTRINSIC, (uint32) intrinsicGCrun);
    objectSetSlot(GC, stringToAtom("isOn"),  PROTO_ACTION_INTRINSIC, (uint32) intrinsicGCisOn);

    //
    //
    // Now initialize the Kit libraries
    //
    // THIS IS A SPECIAL, CODEGENERATED TRICK
    assert(objectGetSlot(Kit, stringToAtom("libraries"), &tmpObj));
    {
	extern uint32 OSSL;

	objectSetSlot(tmpObj, (Atom) stringToAtom("OS"), PROTO_ACTION_INTRINSIC, (uint32) &OSSL);
    }

#include "kitlibs.h"

    // Now create the builtin OS module
    tmpObj = objectNew(0);
    linkLib(tmpObj, "OS");
    objectSetSlot(Globals, (Atom) stringToAtom("OS"), PROTO_ACTION_GET, (uint32) tmpObj);

    GCON();
   

    return 0;
}




// This routine re-sets the C language globals with their proper object
// reference values. (These could change after a garbage collection cycle)
int reassignRoots (Proto lobby) 
{
    Proto tmpObj;


    objectGetSlot(lobby, stringToAtom("DefaultBehavior"), &tmpObj);
    DefaultBehavior = tmpObj;


    // The Globals Chain
    objectGetSlot(lobby, stringToAtom("Globals"), &tmpObj);
    Globals = tmpObj;

    objectGetSlot(Globals,  stringToAtom("true"),  &tmpObj);
    TrueObj = tmpObj;

    objectGetSlot(Globals,  stringToAtom("false"), &tmpObj);
    FalseObj = tmpObj;

    Cpu->trueObj  = (uint32) TrueObj;
    Cpu->falseObj = (uint32) FalseObj;


    objectGetSlot(Globals,  stringToAtom("Kit"), &tmpObj);
    Kit = tmpObj;

    objectGetSlot(Globals, stringToAtom("SimpleDataFactory"), &tmpObj);
    SimpleDataFactory = tmpObj;

    objectGetSlot(Globals, stringToAtom("Capsule"), &tmpObj);
    Capsule = tmpObj;

    objectGetSlot(Globals, stringToAtom("GC"), &tmpObj);
    GC = tmpObj;


    // The Traits Chain
    objectGetSlot(lobby, stringToAtom("Traits"), &tmpObj);
    Traits = tmpObj;

    objectGetSlot(Traits, stringToAtom("Clonable"), &tmpObj);
    Clonable = tmpObj;

    objectGetSlot(Clonable, stringToAtom("ObjectClass"), &tmpObj);
    ObjectClass = tmpObj;
    
    objectGetSlot(Clonable, stringToAtom("StringParent"), &tmpObj);
    StringParent = tmpObj;
    
    objectGetSlot(Clonable, stringToAtom("FloatParent"), &tmpObj);
    FloatParent = tmpObj;
    
    objectGetSlot(Clonable, stringToAtom("ArrayParent"), &tmpObj);
    ArrayParent = tmpObj;
    

    // The OddBalls Chain
    objectGetSlot(Traits, stringToAtom("OddBalls"), &tmpObj);
    OddBalls = tmpObj;

    objectGetSlot(OddBalls, stringToAtom("IntegerClass"), &tmpObj);
    IntegerClass = tmpObj;

    objectGetSlot(OddBalls, stringToAtom("AtomClass"), &tmpObj);
    AtomClass = tmpObj;

    objectGetSlot(OddBalls, stringToAtom("WhileTrueObj"), &tmpObj);
    WhileTrueObj = tmpObj;

    objectGetSlot(OddBalls, stringToAtom("WhileFalseObj"), &tmpObj);
    WhileFalseObj = tmpObj;

    return 0;
}



void * objectNew (int slotNum)
{
    Proto proto;
    
    // Malloc the space for the new object
    // the header for the object is contained as the header in the malloc chunk

    // The minimum size will be 6 slots, this will insure that we hit our fill factor properly
    slotNum = ( slotNum < 6 ? 6 : slotNum);
    
    // Ok, since the slots require 3 bits.the string literal can only take 29

    // The hash needs to know the size - the malloc's size will imply the # of slots for the max
    // table size.- That will be  (size - 4) >> 3   or (size-4)/8

    proto = (Proto) celObjAlloc(4 + (slotNum << 3));

    // Initialize it
    proto->count = 0;

    // Now tag the header of the chunk so it is now a GC'd  'object'/proto
    proto->header |= PROTO_FLAG_GC | PROTO_FLAG_PROTO;

    // Now embed the type into the reference...

    (uint32) proto |= PROTO_REF_PROTO;

    return proto;
    
}

// Allocate an object that can hold 'slotNum' slots
// and 'dataNum' data slots
// Objects are basically done as 2 * the slot count
// Here we know the final size (this routine is usually
// called from sd2cel
void * objectNewWithSize (int dataNum, int slotNum)
{

    int i;

    i = (slotNum > dataNum) ? slotNum : dataNum;

    return (objectNew(i));
}

Proto objectNewInteger (int i)
{
    //// Need to check for overflows - it will happen from sd2cel or people
    //// calling this routine directly

    // Ok, need to shift number to the left //// Hopefully this is signed
    i <<= 3;

    i |= PROTO_REF_INT;

    return (Proto)i;
}

// Creates a new float object with the actual float specified
Proto objectNewFloat (double f)
{
    Proto blob;
    Proto newFloat;
    char * p;


    GCOFF();
    
    newFloat = objectNew(0);

    objectSetSlot(newFloat, ATOMparent, PROTO_ACTION_GET, (uint32) FloatParent);

    // Create a new blob to hold the data
    blob = objectNewBlob(sizeof(double));
    p = objectPointerValue(blob);
    
    objectSetSlot(newFloat, ATOM_float, PROTO_ACTION_GET, (uint32) blob);
    memcpy(p, &f, sizeof(double));

    GCON();


    return newFloat;
}


// Sets an existing float object with a new value
// **kind of a tricky behind the scenes optimization
Proto objectSetFloat (Proto newFloat, double f)
{
    Proto blob;
    char * p;

    assert(objectGetSlot(newFloat, ATOM_float, &blob));
    p = objectPointerValue(blob);
    memcpy(p, &f, sizeof(double));

    return newFloat;
}


// Creates a new string that is a copy of the specified arguments
void * objectNewString(char * str)
{
    Atom p;

    p = UniqueString(str);
    
    return (void *)p;
}


void *  objectNewBlob (int size)
{
    Proto proto;

    proto = (Proto) celObjAlloc(size);
    
    // Initialize it
    // Now tag the header of the chunk so it is now a GC'd  'object'/proto
    proto->header |= PROTO_FLAG_GC | PROTO_FLAG_BLOB;

    // Now embed the type into the reference...
    (uint32) proto |= PROTO_REF_BLOB;

    return ((void *) proto);
}

// An object blob is what stores object references for arrays or hashes
void *  objectNewObjectBlob (int num)
{
    Proto objBlob;
    Proto proto;

    // Build a Blob that can hold num items as capacity
    objBlob = objectNewBlob(num << 2);
    
    // Indicate that the object contains object references, ie. it is
    // clear, not opaque
    proto = PROTO_REFVALUE(objBlob);
    proto->header |= PROTO_FLAG_CLEAR;
    
    return objBlob;
}

// What the size of a blob is
uint32 objectBlobSize (Proto obj)
{
    uint32 size;
    Proto proto;

    // Remove the type 'Blob' from the reference
    proto = PROTO_REFVALUE(obj);
    
    size = proto->header & PROTO_SIZEFIELD;
    size -= 4;			// 4 extra bytes for the malloc header

    return size;
}

// An object blob is what stores object references for array, hashes, or arrays
void *  objectBlobResize (Proto obj, int num)
{
    uchar * p;
    uchar * p2;
    uint32 size;
    Proto objBlob;

    // Build a Blob that can hold num items as capacity
    objBlob = objectNewObjectBlob(num);
    
    p = objectPointerValue(objBlob);
    
    size = objectBlobSize(obj);
    p2 = objectPointerValue(obj);

    memcpy(p, p2, size);
    

    return objBlob;
}

Proto objectBlobAtGet(Proto obj, uint32 index)
{
    uint32 * p;
    
    p = (uint32 *) objectPointerValue(obj);
    return ( (Proto) p[index]);
}

void objectBlobAtSet(Proto obj, uint32 index, Proto newItem)
{
    uint32 * p;
    
    p = (uint32 *) objectPointerValue(obj);
    p[index] = (uint32)newItem;
}


//// This function will change dramatically once the GC is coded
void  objectFree(void * obj)
{
//     Proto proto;
//     char * p;
//     int i;
    
//     p = (char *) obj;

//     // Determine the type of reference this is
//     i = (uint32) proto & PROTO_REF_TYPE;
//     proto = (struct _proto *)((uint32) obj & PROTO_REF_MASK);

//     switch (i) {
// 	case PROTO_REF_ATOM:
// 	case PROTO_REF_INT:
// 	    // Do nothing
// 	    break;
// 	case PROTO_REF_BLOB:
// 	case PROTO_REF_PROTO:

// 	    //// This will be changed dramatically once the sweep phase of the GC is coded
	    
// 	    // Since we are pointing within the malloc chunk header, we must restore the
// 	    // offset so free will work.
// 	    p += 4;
// 	    celfree(p);
// 	    break;
//     }
    return;

}

inline Proto objectSetAsCodeProto(Proto proto)
{
    if ( (PROTO_REFTYPE(proto) ) != PROTO_REF_PROTO) {
	assert(0);
    }

    proto = objectResolveForwarders(proto);

    // Makesure we aren't leaving any type in the pointer
    proto = PROTO_REFVALUE(proto);

    // Needs to be updated
    // Now tag the header of the chunk so it is now a GC'd  'object'/proto

    // Must FLAG the object as code for the GC
    proto->count |= PROTO_COUNT_TYPEISCODE;

    (uint32) proto |= PROTO_REF_CODE;
    return proto;
}

inline Proto objectSetAsBlockProto(Proto proto)
{
    if ( ((uint32) proto & PROTO_REF_TYPE) != PROTO_REF_PROTO) {
	assert(0);
    }

    proto = objectResolveForwarders(proto);

    // Makesure we aren't leaving any type in the pointer
    proto = (struct _proto *)((uint32) proto & PROTO_REF_MASK);
    (uint32) proto |= PROTO_REF_BLOCK;
    return proto;
}



inline int
objectInUse(void * obj)
{
    struct _proto * p1;
    struct _proto * p2;
    int i;
    
    p1 = (struct _proto *) ( (uint32) obj & PROTO_REF_MASK);
    p2 = (struct _proto *) ( (void *) p1 + ((uint32)p1->header & PROTO_SIZEFIELD) + 4 );

    i = p2->header & 1;

    return i;
}


// This resolves objects that are really forwarders
inline void * objectResolveForwarders (void * obj)
{
    struct _proto * p;
    
    while (1) {
	if ( PROTO_REFTYPE(obj) != 0 || (obj == nil)) {
	    return obj;
	}

	p = PROTO_REFVALUE(obj);

	// Is the object a proto ?
	if (PROTO_ISFORWARDER(p)) {
	    obj = PROTO_FORWARDVALUE(p);
	} else {
	    return (void *)( obj );
	}
    }
}


// Accessing type of objects
int32 objectIsType(Proto proto)
{
    int32  i;

    proto = objectResolveForwarders(proto);

    i = ((uint32) proto & PROTO_REF_TYPE); 

    return i;
}


// Accessing objects - key is a reference
void *  objectSetSlot (Proto proto, Atom key, char action, uint32 data)
{
    Proto  p1;
    ProtoSlot slotPtr;

    proto = objectResolveForwarders(proto);

    if (!(( PROTO_REFTYPE(proto) == PROTO_REF_PROTO ) || ( PROTO_REFTYPE(proto) == PROTO_REF_CODE)))
 {
        // Need to throw an exception
        // Block protos and oddballs should not get slots added
        assert(0);
    }

    // Lookup the key - and set a pointer to the slot

    
    if (objectLookupKey(proto, (uint32) key, &slotPtr)) {

	// If the key was found, set the new action and data..
	
	
	p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);
	
	// If it is found, set the data 
	p1->data[slotIndex(slotPtr)] = data;
	// Change the action 
	slotSetAction(slotPtr, action);
	
    } else {
        // if it isn't in there, we must add this variable to the variable array and to the hash

	// Since it will always be the slot count that overflows, we will check that
	// Check to see if hash is full and expand if necessary
	if (( (objectSlotCount(proto) * 10) / objectCapacity(proto) ) >= PROTO_FillFactor) {
	    proto = objectExpand(proto);
	    assert(!objectLookupKey(proto, (uint32) key, &slotPtr));
	}

	p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);

	// Add new slot
	p1->data[protoDataCount(p1)] = data;

	// Find the empty slot for this key so we can put data into it
	assert(!objectLookupKey(proto, (uint32) key, &slotPtr));

	slotSetIndex(slotPtr, protoDataCount(p1));
	
	protoSetDataCount(p1, (protoDataCount(p1)+1)); // Now increment the data index
	slotSetKey(slotPtr, (uint32) key);
	slotSetAction(slotPtr, action);
	protoSetSlotCount(p1, (protoSlotCount(p1)+1)); // Now increment the slot count
	
    }

    return proto;
}

// This routine is for setting the data section for a proto
// It is used for sd2cel and shouldn't be used for normal use
void *  objectSetDataAtIndex(Proto proto, uint32 index, uint32 data)
{
    uint32 i;

    proto = objectResolveForwarders(proto);

    if ( ((uint32) proto & PROTO_REF_TYPE) != PROTO_REF_PROTO) {
	//// Handle the different type
    }


    i = protoDataCount(proto);
    i = i - 1;

    // check the index
    if (index > i) {
      return nil;
    }

    protoDataAtIndex(proto, index) = data;

    return proto;
}


// This routine returns the index of the slot for the data
// it throws an assertion if it can't find the slot
// *This is for experienced people only*
// 
int objectGetDataOffsetForSlot(Proto proto, Atom key)
{
    int i;
    ProtoSlot slotPtr;
    
    proto = objectResolveForwarders(proto);

    if ( ((uint32) proto & PROTO_REF_TYPE) != PROTO_REF_PROTO) {
	//// Handle the different type
	assert(0);
    }

    if (objectLookupKey(proto, (uint32) key, &slotPtr) == 0) {
	assert(0);
    }
    
    i = slotIndex(slotPtr);

    return i;
}


// This routine does an easy add of a slot that is a setter getter
// It assumes that the key is from some source, so it lacks the colon
// Example: 'x <-> 0.' 
void *  objectEasySetSlot(Proto proto, Atom key, uint32 data)
{
    char buff[64];
    
    proto = objectResolveForwarders(proto);

    if ( ((uint32) proto & PROTO_REF_TYPE) != PROTO_REF_PROTO) {
	//// Handle the different type
	assert(0);
    }

    // Overwrite existing slots and create new slot
    objectSetSlot(proto, key, PROTO_ACTION_GET, data);
    
    // Add the colon for the setter
    strcpy(buff, atomToString(key));
    strcat(buff, ":");
    
    objectSetSlotSame(proto, stringToAtom(buff), key, PROTO_ACTION_SET);

    return proto;
}


// This routine does an easy add of a slot that is a setter getter
// It assumes that the key is from some source, so it lacks the colon
// Example: 'x <-> 0.' 
void *  objectEasySetFrameSlot(Proto proto, Atom key, uint32 data)
{
    char buff[64];
    
    proto = objectResolveForwarders(proto);

    if ( ((uint32) proto & PROTO_REF_TYPE) != PROTO_REF_PROTO) {
	//// Handle the different type
	assert(0);
    }

    // Overwrite existing slots and create new slot
    objectSetSlot(proto, key, PROTO_ACTION_GETFRAME, data);
    
    // Add the colon for the setter
    strcpy(buff, atomToString(key));
    strcat(buff, ":");
    
    objectSetSlotSame(proto, stringToAtom(buff), key, PROTO_ACTION_SETFRAME);

    return proto;
}



// This inserts a slot into a proto, which only affects the slot count
// This routine doesn't require the data to be passed in, just the index 
//   to the data.
// It is assumed that the data is already be present. None of the keys should already be present, otherwise it will
// overwrite the key slot. Also, it doesnt check for proto fullness
// It is assumed that those things were done (expansion and no existing slot is present)
// This is for the object passivation libraries and other routines
// internal to this module
void *  objectInsertSlot(Proto proto, uint32 key, uint32 action, uint32 index)
{
    struct _protoEntry * slotPtr;
    struct _proto * p1;
    int i;

    // Notice no check for the resolveForwarder, it shouldn't have to do that
    // I should stick in an ASSERT

    if ( ((uint32) proto & PROTO_REF_TYPE) != PROTO_REF_PROTO) {
	//// Handle the different type
        assert(0);
    }
    p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);

    // Lookup the key - and set a pointer to the slot
    i = objectLookupKey(proto, key, &slotPtr);

    slotSetIndex(slotPtr, index);
    slotSetKey(slotPtr, key);
    slotSetAction(slotPtr, action);

    // If we didn't find the slot increment the slot count
    if (0 == i) {
      protoSetSlotCount(p1, (protoSlotCount(p1)+1));
    }  

    return proto;
}




// Insert a slot that references an existing data item via an existing slot
void *  objectSetSlotSame(Proto proto, Atom key, Atom existingKey, char action)
{
    struct _protoEntry * slotPtr;
    uint32   index;


    proto = objectResolveForwarders(proto);
    
    // Lookup the key - and set a pointer to the slot
    if (objectLookupKey(proto, (uint32) existingKey, &slotPtr)) {

	// Ok, get a copy of the data index for that key
	index   = slotIndex(slotPtr);

	// Check to see if hash is full and expand if necessary
	if (( (objectSlotCount(proto) * 10) / objectCapacity(proto) ) >= PROTO_FillFactor) {
	    proto = objectExpand(proto);
	}

	objectInsertSlot(proto, (uint32) key, action, index);
	
    } else {
	return nil;
    }

    return proto;
}



// Lookup the key in the hash table, if it isn't found (it will hit a nil) entry
// This routine sets the slotptr to the last slot it looked at
inline int objectLookupKey (Proto proto, uint32 key, struct _protoEntry * * slotPtr)
{
    int i, j, k;
    struct _proto * p1;
    

    if ( PROTO_REFTYPE_REF(proto) || (PROTO_REFTYPE(proto) == PROTO_REF_BLOB) ) {
	//// Handle the different type
        assert(0);
    }
    p1 = PROTO_REFVALUE(proto);

    //// check the types - like the key

    ////// This is a poor hash - must replace after analysis

    key &= PROTO_SLOT_KEYMASK;

    j = objectCapacity(p1);
    
    i = (key >> 11) % j;

    k = 0;
    

    while (1) {
	*slotPtr = (struct _protoEntry *) &(p1->data[(j + i)]);
	if ( ( (*slotPtr)->keyActionIndex & PROTO_SLOT_KEYMASK) == key ) {
	    return 1;		// it was found, return it
	} 
	// If it is the null field, then stop
	if ( (*slotPtr)->keyActionIndex == 0 ) {
	    return 0;
	}

	i++;
	if (i == j) {
	    i = 0;
	}
	// increment the counter
	k++;
	// watch for runaway proto's
	assert(k < j);
	
    }
}


// This will expand a proto, by creating a new proto with double the capacity and then
// copying the contents of the original into the new proto. This should only be called
// from the objectSetSlot routine
void * objectExpand (struct _proto * oldProto)
{
    struct _proto * newProto;
    struct _proto * p;
    struct _protoEntry * slotPtr;
    int i;
    int oldType;
    

    if ( PROTO_REFTYPE_REF(oldProto) || (PROTO_REFTYPE(oldProto) == PROTO_REF_BLOB) ) {
	//// Handle the different type
        assert(0);
    }

    oldType = PROTO_REFTYPE(oldProto);
    // Normalize the proto to a PROTO (as opposed to CODE or BLOCK)
    oldProto = PROTO_REFVALUE(oldProto);

    // Ok, lets double the proto's size
    newProto = objectNew(objectCapacity(oldProto) << 1);

    p = (struct _proto *)((uint32)newProto & PROTO_REF_MASK);

    // First copy the instance data (it's counts
    protoSetDataCount(p, protoDataCount(oldProto));
    memcpy( p->data, oldProto->data, (objectDataCount(oldProto) << 2) );

    
    // Now lets add the contents of this hash into the other
    // hash (iterate through a proto accessing it's slots, then
    // simultaneously, add them to the new proto.
    // Since the data is already there, we use a special objectInsertSlot()
    // function that just add's the slot's and doesn't affect the data count
    // It does affect the slot count

    i = 0;
    while (1) {
	slotPtr = objectEnumerate(oldProto, &i, 0);
	if (slotPtr == nil) {
	    break;
	}
	objectInsertSlot(newProto, slotKey(slotPtr), slotAction(slotPtr), slotIndex(slotPtr));
    }

    // Now set the original proto as a forwarder
    // Need to make this atomic (maybe do it as a long long store?)
    oldProto->data[0] = (uint32) p;      // have the count be the pointer to the data
    oldProto->count |= PROTO_COUNT_FORWARDER; // mark the object as a forwarder
    
    // Restore the type
    (uint32) newProto |= oldType;
    return newProto;
}

// This will clone a proto,
void * objectClone (Proto proto)
{
    Proto      newProto;
    Proto      srcProto;
    Proto      p;
    ProtoSlot  slotPtr;
    int i;
    
    proto = objectResolveForwarders(proto);
    
    // We need to get the proto's value since we access
    // the object directly and the TYPE info would offset
    // the pointer improperly
    srcProto = PROTO_REFVALUE(proto);
    i        = PROTO_REFTYPE(proto);

    switch (i) {
	case PROTO_REF_INT:
	case PROTO_REF_ATOM:
	case PROTO_REF_BLOB:
	    return proto;
	    break;
	    
	case PROTO_REF_CODE:
	    // Need to handle this
	    assert(0);
	    
	case PROTO_REF_PROTO:
	case PROTO_REF_BLOCK:
	    newProto = objectNew(objectCapacity(srcProto));
	    
	    p = (struct _proto *)((uint32)newProto & PROTO_REF_MASK);
	    
	    // First copy the instance data (it's counts
	    protoSetDataCount(p, protoDataCount(srcProto));
	    memcpy( p->data, srcProto->data, (objectDataCount(srcProto) << 2) );
	    
	    
	    // Now lets add the contents of this hash into the other
	    // hash (iterate through a proto accessing it's slots, then
	    // simultaneously, add them to the new proto.
	    // Since the data is already there, we use a special objectInsertSlot()
	    // function that just add's the slot's and doesn't affect the count
	    
	    i = 0;
	    while (1) {
		slotPtr = objectEnumerate(srcProto, &i, 0);
		if (slotPtr == nil) {
		    break;
		}
		objectInsertSlot(newProto, slotKey(slotPtr), slotAction(slotPtr), slotIndex(slotPtr));
	    }

	    return newProto;
	    
	    break;
    }

    return nil;			// Make the compiler happy
}

    
// Enumerating through slots 
ProtoSlot objectEnumerate (Proto proto, int * currentSlot, int skipParent)
{
    struct _protoEntry * slotPtr;
    int i,j;

    if ( ! PROTO_REFTYPE_PROTO(proto) ) {
	//// Handle the different type
        assert(0);
    }

    // This routine should never have to deal with a forwarder
    if (PROTO_ISFORWARDER(proto)) {
	assert(0);
    }
    
    
    i = objectSlotCount(proto);

    // If it is an empty proto, return
    if (i == 0) {
	return nil;
    }
    
    j = objectCapacity(proto);

    // Normalize the proto reference to an actual pointer
    proto = PROTO_REFVALUE(proto);

    // From the currentSlot number, start looking for a slot
    while (1) {

	// If we are past the end, return nil, the end-of-proto indicator
	if (*currentSlot >= j) {
	    return nil;
	} else {
	    slotPtr = (struct _protoEntry *)&( proto->data[(j  + *currentSlot)] );
	    (*currentSlot)++;	// Increment the counter
	    if (skipParent && ((Atom)slotKey(slotPtr) == ATOMparent)) {
		continue;
	    }
	    
	    // The sentinel for a deleted slot is just 0x01
	    // So this would return a slot if it was not a sentinel
	    if ((slotPtr->keyActionIndex & 0xFFFFFFFE) != 0) {
		return slotPtr;
	    }
	}
    }
}



// Lookup the key in the hash table, if it isn't found (it will hit a nil) entry
// Return the data value for the slot
uint32 objectGetSlot (Proto proto, Atom key, Proto * result)
{
    int        i,k;
    Proto      p1;
    ProtoSlot  slotPtr;
    
    

    proto = objectResolveForwarders(proto);
    
    if ( ! PROTO_REFTYPE_PROTO(proto) ) {
	//// Handle the different type
        assert(0);
    }

    p1 = PROTO_REFVALUE(proto);

    assert(key != nil);
    
    i = objectLookupKey(p1, (uint32) key, &slotPtr);

    if (i) {
	// Check the type 
	k = slotAction(slotPtr);

	switch (k) {
	    case PROTO_ACTION_PASSTHRU:
	    case PROTO_ACTION_GET:
	    case PROTO_ACTION_INTRINSIC:		// This is here for kit imports
							// Intrinsics have become my way
							// of storing 32 bits of data
						        // like C pointers
							// This is dangerous
		*result = (Proto) protoSlotData(p1, slotPtr);
                return 1;
		break;
	    case PROTO_ACTION_GETFRAME:
		*result =  (Proto) getFrameAt(Cpu, objectIntegerValue( protoSlotData(p1, slotPtr)) );
                return 1;
		break;

		// These shouldn't be called for a getSlot
	    case PROTO_ACTION_SET:
	    case PROTO_ACTION_SETFRAME:
		assert(0);
		break;
		
	}
    } 

    // The slot wasn't found
    return 0;
}

// Lookup the key in the hash table, if it isn't found (it will hit a nil) entry
// If it is found, remove the slot
uint32 objectDeleteSlot (Proto proto, Atom key)
{
    int        i;
    Proto      p1;
    ProtoSlot  slotPtr;
    
    
    proto = objectResolveForwarders(proto);
    
    if ( ! PROTO_REFTYPE_PROTO(proto) ) {
	//// Handle the different type
        assert(0);
    }

    p1 = PROTO_REFVALUE(proto);

    i = objectLookupKey(p1, (uint32) key, &slotPtr);

    if (i) {
	// Determine if this is the only one pointing
	// to that index

	//// SCRATCH THAT, 
	//// 
	//// * WE'LL JUST GARBAGE COLLECT THEM LATER
	////   MAYBE WE'LL DO THAT LATER

	slotPtr->keyActionIndex = 1;
	
	// Two choices: rehash everything, kind of expensive for a delete
	//              leave a sentinel, requires a bit more work for clones, expands
	//              but is very inexpensive

	// We'll leave a sentinel, it's easy
	//protoSetSlotCount(p1, (protoSlotCount(p1)-1));


	return 1;

    } 

    // The slot wasn't found
    return 0;
}


// Return the pointer to a blob's area
void * objectPointerValue (Proto proto)
{
    Proto p1;
    char * p;
    int  i;
    
    
    // Set the default
    p = (void *) 0;

    proto = objectResolveForwarders(proto);
    
    i = PROTO_REFTYPE( proto );
    switch (i) {

	// These shouldn't receive this call
	case PROTO_REF_INT:
	case PROTO_REF_PROTO:
	    p = nil;
	    break;
	    
	case PROTO_REF_BLOB:
	    p1 = PROTO_REFVALUE(proto);
	    p = (char *) p1;
	    // Ofset the pointer into the data
	    p += 4;
	    break;
	    
	case PROTO_REF_ATOM:
	    p = atomToString((Atom)proto);
	    break;
    }

    return p;
	
}

// Return the value of an integer reference
uint32 objectIntegerValue (Proto proto)
{
    int i;
    struct _proto      * p1;
    

    proto = objectResolveForwarders(proto);
    
    if ( PROTO_REFTYPE(proto) != PROTO_REF_INT) {
	//// Handle the different type
        assert(0);
    }

    p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);

    
    i = (int) p1;

    // Bring the integer back
    i >>= 3;

    return (i);
}



// Return the value of a float object
double objectFloatValue (Proto proto)
{
    double f;
    int i;
    Proto tmp;
    double * p;
    

    proto = objectResolveForwarders(proto);
    
    if ( PROTO_REFTYPE(proto) == PROTO_REF_INT) {
	f = objectIntegerValue(proto);
    } else {
        i = objectGetSlot(proto, ATOM_float, &tmp);
	assert(i);
	p = (double *) objectPointerValue(tmp);
	f = *p;
    }
    
    return (f);
}




// Return the number of data or slot spaces  in the proto 
inline int     objectCapacity (Proto proto)
{
    if ( ! PROTO_REFTYPE_PROTO(proto) ) {
	//// Handle the different type
        assert(0);
	return -1;
    }
    proto = PROTO_REFVALUE(proto);
    // TRICKY - Since we alloc with 4 + (slot * 8) 
    //          malloc really add's 4 to that
    //          So we have size = 4 + (the above)
    //          This simplifies into  header/8 = slots
    return ( ((proto->header & PROTO_SIZEFIELD) - 8) >> 3);
}


// Return the number of slots in the proto
inline int     objectSlotCount(Proto proto)
{
    if ( ! PROTO_REFTYPE_PROTO(proto) ) {
	//// Handle the different type
        assert(0);
	return -1;
    }
    return ( protoSlotCount(proto) );
}


// Return the number of slots in the proto
inline int     objectDataCount(Proto proto)
{
    if ( ! PROTO_REFTYPE_PROTO(proto) ) {
	//// Handle the different type
        assert(0);
	return -1;
    }
    return ( protoDataCount(proto) );
}



// Object dump
void * objectDump (Proto proto)
{
    int        i,j,k;
    ProtoSlot  slotPtr;
    Proto      p1;
    Proto      tmp;
    char       buff[2048];

    // Determine the type of reference this is
    i  = PROTO_REFTYPE(proto);
    p1 = PROTO_REFVALUE(proto);

    if (proto == nil) {
	sprintf(buff, " <nil> ");
	printString(buff);
	return proto;
    }
    

    switch (i) {
	case PROTO_REF_INT:
	    // display the int
	    sprintf(buff, "(%ld) Integer", objectIntegerValue(proto));
	    printString(buff);
	    break;
	case PROTO_REF_PROTO:
	case PROTO_REF_BLOCK:
	case PROTO_REF_CODE:
	    if (PROTO_ISFORWARDER(proto)) {
		// Indicate that this is a forwarder
		sprintf(buff, "[0x%lx] Forwarder ->", (uint32) proto);
		printString(buff);
		objectDump(PROTO_FORWARDVALUE(proto));
	    } else 
	    if (proto == TrueObj) {
		printString("true\n");
		break;
	    } else 
	    if (proto == FalseObj) {
		printString("false\n");
		break;
	    } else {
		printString("{");
		addPrintSpaces(4);
		printString("\n");

		i = objectGetSlot(proto, ATOM_array, &tmp);
		if (i) {
		    printString("Name: ");
		    objectGetSlot(proto, stringToAtom("name"), &tmp);
		    sprintf(buff, "[%s] ", atomToString((Atom) tmp));
		    printString(buff);
		    
		    printString("Size: ");
		    objectGetSlot(proto, ATOMsize, &tmp);
		    i = objectIntegerValue(tmp);
		    sprintf(buff, "[%d]\n", i);
		    printString(buff);

		    j = objectGetSlot(proto, stringToAtom("_annotate"), &tmp);
		    if (j) {
			printString("Annotate");
			objectDump(tmp);
		    }
		    
		    
		    // enumerate through the proto showing it's contents
		    for (j = 0; j < i; j++) {
			sprintf(buff, "\"%d\" <- ", j);
			printString(buff);
			
			// Access the data and print it
			arrayGet(proto, j, &tmp);
			objectDump(tmp);
			printString("\n");
		    }
		    addPrintSpaces(-4);
		    printString("}");
		    break;
		}

		i = objectGetSlot(proto, ATOM_string, &tmp);
		if (i) {
		    printString("String: ");
		    objectGetSlot(proto, ATOMsize, &tmp);
		    i = objectIntegerValue(tmp);

		    objectGetSlot(proto, ATOM_string, &tmp);
		    ioOut(objectPointerValue(tmp), i);
		}
		else {
		    // enumerate through the proto showing it's contents
		    j = 0;
		    while (1) {
			slotPtr = objectEnumerate(proto, &j, 0);
			if (slotPtr == nil) {
			    break;
			}
			sprintf(buff, "\"%s\" (%lx:%d) ->", atomToString((Atom)slotKey(slotPtr)), slotKey(slotPtr), j );
			printString(buff);
			k = slotAction(slotPtr);
			sprintf(buff, " (%s:%d) -> ", celActionTypes[k], (int)slotIndex(slotPtr) );
			printString(buff);
			switch (k) {
			    case PROTO_ACTION_GET:
			    case PROTO_ACTION_SET:
			    case PROTO_ACTION_PASSTHRU:
				// Access the data and print it
				objectDump(protoSlotData(proto, slotPtr));
				break;
			    case PROTO_ACTION_GETFRAME:
			    case PROTO_ACTION_SETFRAME:
				// Show the For now just say stack
				tmp = protoSlotData(proto, slotPtr);
				assert(PROTO_REFTYPE(tmp) == PROTO_REF_INT);
				sprintf(buff, "FP[0x%lx]", objectIntegerValue(tmp));
				printString(buff);
				break;
			}
		    
			printString("\n");
		    }
		    addPrintSpaces(-4);
		    printString("}");
		}
	    }
	    
	    
	    break;
	case PROTO_REF_BLOB:
	    //// Need to print the address in hex
	    sprintf(buff, "[0x%lx] Blob", (uint32) proto);
	    printString(buff);
	    break;
	case PROTO_REF_CONTEXT:
	    sprintf(buff, "[0x%lx] Context", ((uint32) PROTO_REFVALUE(proto) >> 1));
	    printString(buff);
	    break;
	case PROTO_REF_ATOM:
	    sprintf(buff, "\"%s\"", atomToString((Atom)proto) );
	    printString(buff);
	    break;
    }

    return proto;
    
}

// Object dump
void * objectShallowDump (Proto proto)
{
//    void * p;
    int i,j,k;
    ProtoSlot slotPtr;
    Proto p1;
    Proto data;
    Proto tmp;
    Proto tmpObj;
    double f;
    char buff[256];

    // Determine the type of reference this is
    i  = PROTO_REFTYPE(proto);
    p1 = PROTO_REFVALUE(proto);

    if (proto == nil) {
	sprintf(buff, " <nil> ");
	printString(buff);
	return proto;
    }
    

    switch (i) {
	case PROTO_REF_INT:
	    // display the int
	    sprintf(buff, "(%ld) Integer", objectIntegerValue(proto));
	    printString(buff);
	    break;
	case PROTO_REF_PROTO:
	case PROTO_REF_BLOCK:
	case PROTO_REF_CODE:
	    if (PROTO_ISFORWARDER(proto)) {
		// Indicate that this is a forwarder
		sprintf(buff, "[0x%lx] Forwarder ->", (uint32) proto);
		printString(buff);
		objectShallowDump(PROTO_FORWARDVALUE(proto));
	    } else {
		printString("\n{\n");
		// enumerate through the proto showing it's contents
		j = 0;
		while (1) {
		    slotPtr = objectEnumerate(proto, &j, 0);
		    if (slotPtr == nil) {
			break;
		    }
		    sprintf(buff, " \"%s\" ->", atomToString((Atom)slotKey(slotPtr)) );
		    printString(buff);
		    k = slotAction(slotPtr);
		    sprintf(buff, " (%s:%d) -> ", celActionTypes[k], (int)slotIndex(slotPtr) );
		    printString(buff);
		    switch (k) {
			case PROTO_ACTION_GET:
			case PROTO_ACTION_SET:
			case PROTO_ACTION_PASSTHRU:
			    // Access the data and print it
			    data = protoSlotData(proto, slotPtr);
			    i = PROTO_REFTYPE(data);
			    if (data == nil) {
				sprintf(buff, " <nil> ");
				printString(buff);
			    } else if (PROTO_REF_PROTO == i) {
				tmpObj = protoSlotData(proto, slotPtr);
				i = objectGetSlot(tmpObj, ATOM_string, &tmp);
				if (i) {
				    printString("''");
				    objectGetSlot(tmpObj, ATOMsize, &tmp);
				    i = objectIntegerValue(tmp);
				    
				    objectGetSlot(tmpObj, ATOM_string, &tmp);
				    ioOut(objectPointerValue(tmp), i);
				    printString("''");
				    break;
				} 

				i = objectGetSlot(tmpObj, ATOM_float, &tmp);
				if (i) {
				    f = objectFloatValue(tmpObj);
				    sprintf(buff, "%f", f);
				    printString(buff);
				    break;
				}

				sprintf(buff, "[0x%lx] Proto", (uint32) data);
				printString(buff);
				break;
			    }
			    else if (PROTO_REF_CODE == i) {
				sprintf(buff, "[0x%lx] Code", (uint32) data);
				printString(buff);
			    }
			    else {
				objectShallowDump(data);
			    }
			    break;
			case PROTO_ACTION_GETFRAME:
			case PROTO_ACTION_SETFRAME:
			    // Show the For now just say stack
			    data = protoSlotData(proto, slotPtr);
			    assert(PROTO_REFTYPE(data) == PROTO_REF_INT);
			    sprintf(buff, "FP[0x%lx]", objectIntegerValue(data));
			    printString(buff);
			    break;
		    }
		    
		    printString("\n");
		}
		printString("}\n");
	    }
	    
	    break;
	case PROTO_REF_BLOB:
	    //// Need to print the address in hex
	    sprintf(buff, "[0x%lx] Blob", (uint32) proto);
	    printString(buff);
	    break;
	case PROTO_REF_CONTEXT:
	    sprintf(buff, "[0x%lx] Context", (uint32) proto);
	    printString(buff);
	    break;
	case PROTO_REF_ATOM:
	    sprintf(buff, "\"%.250s\"", atomToString((Atom)proto) );
	    printString(buff);
	    break;
    }

    return proto;
    
}


// pushHook - this is a hook into the VM's pushImmediate instruction
//            so we can clone a block in order to give it a pointer
//            to the context
//
unsigned int pushHook ( VM32Cpu * pcpu, unsigned int val) {

  Proto     codeProto;
  uint32    context;

  if (PROTO_REFTYPE(val) == PROTO_REF_BLOCK) {
     // See if we run into problems with the alignment
     assert( (((pcpu->fp << 1) & PROTO_REF_TYPE) == 0) );

     GCOFF();
     
     codeProto = objectClone( (Proto) val);

     // Set the parent to be the context
     context = ((pcpu->fp << 1) & PROTO_REF_MASK) | PROTO_REF_CONTEXT;
     objectSetSlot(codeProto, ATOMparent, PROTO_ACTION_GET, (uint32) context);
     // Although this proto can never be addressed, we keep a link to it
     objectSetSlot(codeProto, ATOMblockParent, PROTO_ACTION_GET, (uint32) val);

     // Set it to be a code type
     val = ((uint32) codeProto | PROTO_REF_CODE);

     GCON();
  }

  return val;

}



// showArguments
//   This routine attempts to show arguments in the current stack frame
//   It does this by concatenating to a string passed in.
//

void showArguments (char * buff)
{
    
    Proto     tmpObj;
    int       argCount;
    int       i;
    char      localBuff[128];
    int       size;
    char *    p;
    
    
    argCount      =  objectIntegerValue((Proto) stackAt(Cpu, 1));


    argCount = argCount - 2;

    // DRUDRU - once we fix all of the argCounts, we'll remove this
    if (argCount < 0) {
	argCount = 0;
    }
    
    assert(argCount >= 0);
    

    while (argCount != 0) {
	
	tmpObj = (Proto) stackAt(Cpu, (3 + argCount));
	argCount--;
	
	
	i = PROTO_REFTYPE(tmpObj);
	
	switch (i) {
	    case PROTO_REF_INT:
		sprintf(localBuff, "%d ", (int) objectIntegerValue(tmpObj));
		break;
	    case PROTO_REF_ATOM:
		sprintf(localBuff, "A-%.16s", atomToString((Atom)tmpObj));
		break;
	    case PROTO_REF_PROTO:
		if (tmpObj == nil) {
		    strcpy(localBuff, "nil ");
		} else {
		    if (isStringOrAtom(tmpObj)) {
			getStringPtr(tmpObj, &p, &size);
			if (size > 16) {
			    size = 16;
			}
			localBuff[0] = 'S';
			localBuff[1] = '-';
			memcpy(localBuff+2, p, size);
			localBuff[size+2] = ' ';
			localBuff[size+3] = 0;
		    } else if (tmpObj == TrueObj) {
			strcpy(localBuff, "'true' ");
		    } else if (tmpObj == FalseObj) {
			strcpy(localBuff, "'false' ");
		    } else {
			// DRUDRU - maybe see if it has a 'type' slot and show that
			// DRUDRU - maybe see if it there was a 'debugName'
			strcpy(localBuff, "PROT ");
		    }
		}
		break;
	    case PROTO_REF_CODE:
		strcpy(localBuff, "CODE ");
		break;
	    case PROTO_REF_BLOCK:
		strcpy(localBuff, "BLOK ");
		break;
	    case PROTO_REF_CONTEXT:
		strcpy(localBuff, "CTXT ");
		break;
	}

	strcat(buff, localBuff);
    }
    strcat(buff, "\n");
}
    


// activateSlot is the core routine of this language
//   it is what causes routines to be called
//   it takes it's arguments off of the virtual stack and searches
//   for the proper method
//

void activateSlot (void)
{
    
    Atom      message;
    Proto     initialTarget;
    uint32    initialFP;
    uint32    newFP;
    Proto     target;
    ProtoSlot slotPtr;
    int       ttl = 32;
    int       i,j,k;
    Proto     argCount;
    int       seenDefaultBehavior = 0;
    int       seenLobby           = 0;
    char      buff[128];
    uint32    obj;
    void      (*func)(void);
// To turn this back on, remove the string OFF
// SEE BELOW, this is tied to one more ifdef
#ifdef unixOFF
    struct timeval tv;
    struct timeval difftv;
#endif
    
    
    
    
    // Ok, get the initialTarget object and the method off of the stack
    message       = (Atom)  stackAt(Cpu, 3);
    initialTarget = (Proto) stackAt(Cpu, 2);
    argCount      = (Proto) stackAt(Cpu, 1);

    // Setup the fp
    initialFP =  Cpu->fp;
    newFP     =  Cpu->fp;
    

    if (runtimeActivateTrace) {

	if (debugTime) {
// To turn this back on, remove the string OFF
#ifdef unixOFF
	    gettimeofday(&tv, NULL);
	    timersub(&tv, &lasttv, &difftv);
	    // Insert code to print relative time info here
	    sprintf(buff, "\n<<%ld.%06ld>>\n", difftv.tv_sec, difftv.tv_usec);
	    printString(buff);
	    lasttv = tv;
#endif
	}
	
	target = objectResolveForwarders(initialTarget);
	
	i = (uint32) target & PROTO_REF_TYPE;
	
	switch (i) {
	    case PROTO_REF_INT:
	        sprintf(buff, " INT  [%10d] Sent: %s ", (int) objectIntegerValue(target), atomToString(message));
		showArguments(buff);
		break;
	    case PROTO_REF_ATOM:
	        sprintf(buff, " [Atom: %.50s] Sent: %s argCount:%d\n", atomToString((Atom)target), atomToString(message), (int) objectIntegerValue(argCount));
		break;
	    case PROTO_REF_PROTO:
		if (target == nil) {
		    sprintf(buff, "  nil  Sent: %s ", atomToString(message));
		    showArguments(buff);
		} else {
		    //j = objectLookupKey(target, (uint32) DebugName, &slotPtr);
//		    j = 0;
//	    if (j) {
//			objectGetSlot(target, DebugName, &tmpObj);
//			sprintf(buff, "      [%s] Sent: %s argCount:%d\n", atomToString((Atom)tmpObj), atomToString(message), (int) objectIntegerValue(argCount));
//		    } else {
			sprintf(buff, "      [0x%08lx] Sent: %s ", (uint32) target, atomToString(message));
			showArguments(buff);
//		    }
		}
		break;
	    case PROTO_REF_CODE:
   		sprintf(buff, " CODE [0x%08lx] Sent: %s ", (uint32) target, atomToString(message));
		showArguments(buff);
		break;
	    case PROTO_REF_BLOCK:
   		sprintf(buff, " BLOK [0x%08lx] Sent: %s argCount:%d\n", (uint32) target, atomToString(message), (int) objectIntegerValue(argCount));
		break;
	    case PROTO_REF_CONTEXT:
   		sprintf(buff, " CTXT [0x%08lx] Sent: %s argCount:%d\n", (uint32) target, atomToString(message), (int) objectIntegerValue(argCount));
		break;
        }

	printString(buff);
    }
    
    if (nil == initialTarget) {
	if (message == stringToAtom("isNil")) {

	    // SAME CODE AS ACTION INTRINSIC
	    // Ok, get the data for the object
	    obj = (uint32) intrinsicObjectIsNil;
	    
	    func =  ( void (*)(void) ) (obj);
	    
	    (*func)();			// Call the function

	    return;
	    
	} else {
	    sprintf(buff, "target is <nil> object. Can't send messages to nil.\n");
	    printString(buff);
	    exit(1);
	}
    }
    

//      if (message == stringToAtom("print")) {
//  	target = nil;
//      }
    
    
    target = initialTarget;

    while (1) {
	
	
	target = objectResolveForwarders(target);
	
	i = (uint32) target & PROTO_REF_TYPE;
	
	switch (i) {
	    case PROTO_REF_INT:
		target = IntegerClass;
		continue;
	    case PROTO_REF_ATOM:
		target = AtomClass;
		continue;
	    case PROTO_REF_PROTO:
	    case PROTO_REF_CODE:
		
		j = objectLookupKey(target, (uint32) message, &slotPtr);

		// If it is found
		if (j) {
		    k = slotAction(slotPtr);
		    
		    switch (k) {
			case PROTO_ACTION_GET:
			    actionGet(target, slotPtr);
			    break;
			case PROTO_ACTION_SET:
			    actionSet(target, slotPtr);
			    break;

			case PROTO_ACTION_GETFRAME:
			    actionGetStack(target, slotPtr, newFP);
			    break;
			case PROTO_ACTION_SETFRAME:
			    actionSetStack(target, slotPtr, newFP);
			    break;

			case PROTO_ACTION_PASSTHRU:
			    // Target is where we found the slot, but
			    // all of the actions on the 'self' should
			    // be on the initial target
			    actionPassthru(target, slotPtr, initialTarget);
			    break;
			case PROTO_ACTION_INTRINSIC:
			    actionIntrinsic(target, slotPtr);
			    break;
		    }
		    return;	// Since slot was found, we're done
		}
		// If the slot is not found
		else {
		    // Look for the parent
		    j = objectLookupKey(target, (uint32) ATOMparent, &slotPtr);

		    // If there is a parent
		    if (j) {
			ttl--;
			if (ttl) {
			    // This needs to use the newFP
			    k = slotAction(slotPtr);
			    if (k == PROTO_ACTION_GETFRAME) {
				k = (int) protoSlotData(target, slotPtr);
				k = objectIntegerValue( (Proto) k );
				target = (Proto) getFPRelativeAt(newFP, k);
			    } else {
				objectGetSlot(target, ATOMparent, &target);
			    }
			    
			    
			    // If this is a context reference, then we 
			    // need to use the set the newFP to that context
			    // and get the code-pointer as the new target

			    if (PROTO_REFTYPE(target) == PROTO_REF_CONTEXT) {
				newFP = ((uint32) target & PROTO_REF_MASK) >> 1;
				// Assert that this is an active context
				assert( (initialFP <= newFP) );
				target = (Proto) getFPRelativeAt(newFP, 2);
			    }
			}
			else {
			    sprintf(buff, "too many hops - method not found %s\n", atomToString(message));
			    printString(buff);
			    exit(2);
			}
			    
			continue;
		    }

		    // if there isn't a parent, try the DefaultBehavior
		    // ... if we have already done this, then a method
		    // ... really can't be found so error out
		    else {
			// If we haven't looked at the globals
			if (!seenLobby) {
			    seenLobby = 1;
			    target = Lobby;
			    continue;
			}
			if (seenDefaultBehavior) {
			    sprintf(buff, "method not found %s\n", atomToString(message));
			    printString(buff);
			    objectShallowDump(initialTarget);
			    exit(3);
			}
			
			seenDefaultBehavior = 1;
			target = DefaultBehavior;
			continue;
		    }
		}
		break;
		
	} // switch
	
    } // while
    
}


// locateObjectWithSlotVia
//    This is used by some intriniscs to lookup a slot via inheritance
//    since and intrinsic will sometimes have as a target the context
//    for example (at: 5) print. Will use the 'self' which will be
//    block or code context. I don't want to force people to do '(self at: 5) print.'
//    just yet. This is a sticky issue.
//

Proto locateObjectWithSlotVia (Proto key, Proto obj)
{
    
    Atom      message;
    Proto     initialTarget;
    uint32    initialFP;
    uint32    newFP;
    Proto     target;
    ProtoSlot slotPtr;
    int       ttl = 32;
    int       i,j,k;
    char      buff[128];
    
    
    // Ok, get the initialTarget object and the method off of the stack
    message       = (Atom)  key;
    initialTarget = obj;
    

    // Setup the fp
    initialFP =  Cpu->fp;
    newFP     =  Cpu->fp;
    

    if (nil == initialTarget) {
	sprintf(buff, "target is <nil> object. Can't locate messages via nil.\n");
	printString(buff);
	exit(1);
    }
    
    target = initialTarget;

    while (1) {
	
	target = objectResolveForwarders(target);
	
	i = (uint32) target & PROTO_REF_TYPE;
	
	switch (i) {
	    case PROTO_REF_INT:
		target = IntegerClass;
		continue;
	    case PROTO_REF_ATOM:
		target = AtomClass;
		continue;
	    case PROTO_REF_PROTO:
	    case PROTO_REF_CODE:
		
		j = objectLookupKey(target, (uint32) message, &slotPtr);
		// If it is found
		if (j) {
		    return target;
		}
		// If the slot is not found
		else {
		    // Look for the parent
		    j = objectLookupKey(target, (uint32) ATOMparent, &slotPtr);

		    // If there is a parent
		    if (j) {
			ttl--;
			if (ttl) {
			    // This needs to use the newFP
			    k = slotAction(slotPtr);
			    if (k == PROTO_ACTION_GETFRAME) {
				k = (int) protoSlotData(target, slotPtr);
				k = objectIntegerValue( (Proto) k );
				target = (Proto) getFPRelativeAt(newFP, k);
			    } else {
				objectGetSlot(target, ATOMparent, &target);
			    }
			    
			    // If this is a context reference, then we 
			    // need to use the set the newFP to that context
			    // and get the code-pointer as the new target

			    if (PROTO_REFTYPE(target) == PROTO_REF_CONTEXT) {
				newFP = ((uint32) target & PROTO_REF_MASK) >> 1;
				// Assert that this is an active context
				assert( (initialFP <= newFP) );
				target = (Proto) getFPRelativeAt(newFP, 2);
			    }
			}
			else {
			    sprintf(buff, "too many hops - method not found %s\n", atomToString(message));
			    printString(buff);
			    exit(2);
			}
			    
			continue;
		    }
		    // if there isn't a parent, bail
		    else {
			return nil;
		    }
		}
		break;
		
	} // switch
	
    } // while
    
}




// actionGet assumes that this slot was a 'GET' action. It assumes that the stack
// contained some destination object and the message activated this slot
//

inline void actionGet(Proto object, ProtoSlot slotPtr)
{
    Proto obj;
    
    // No frame is setup like in code protos, it is just a matter of getting
    // the data and returning it

    // Ok, get the data for the object
    obj = protoSlotData(object, slotPtr);
    

    // Now set that as the return value
    VMReturn(Cpu, (unsigned int) obj, 3);
    
}


// actionSet assumes that this slot was a 'SET' action. It assumes that the stack
// contained some destination object and the message activated this slot and the
// new object
//

inline void actionSet(Proto object, ProtoSlot slotPtr)
{
    // Ok, get the data for the object
    protoSlotData(object, slotPtr) = stackAt(Cpu,4);
    
    // Now return the object
    VMReturn(Cpu, (unsigned int) object, 4);
}

// actionGetStack assumes that this slot was a 'GET' action. It assumes that the stack
// contained some destination object and the message activated this slot
//

inline void actionGetStack(Proto object, ProtoSlot slotPtr, uint32 fp)
{
    uint32 obj;
    int i;
    
    // No frame is setup like in code protos, it is just a matter of getting
    // the data and returning it

    i = (int) protoSlotData(object, slotPtr);
    i = objectIntegerValue( (Proto) i );

    // Ok, get the data at that frame position
    obj = getFPRelativeAt(fp, i);
    
    // Now set that as the return value
    VMReturn(Cpu, obj, 3);
}


// actionSetStack locates the argument in the frame that has to be set and
// sets it to the argument passed in via the immediate stack. For example,
// imagine a local variable (which is stack mapped) in a code proto needs
// to get set. The trick is that the data for the set is in the immediate
// stack frame and the variable is relative to the fp
// now for the two lines of code...:)
//

inline void actionSetStack(Proto object, ProtoSlot slotPtr, uint32 fp)
{
    int i;

    i = (int) protoSlotData(object, slotPtr);
    i = objectIntegerValue( (Proto) i );

    // Ok, get the data for the frame off of the stack
    setFPRelativeAt(fp, i, stackAt(Cpu,4));
    
    // Now return the object
    VMReturn(Cpu, (unsigned int) object, 4);
}



// actionPassthru deals with activating some code for a passthru slot
//

inline void actionPassthru (Proto object, ProtoSlot slotPtr, Proto target)
{
    Proto obj;
    char * p;
    Proto codeProto;
    uint32 fp;
    
    
    // The 'object' of this message is the one that has this PASSTHRU
    // slot. However, the target is the proper parent to the code-proto.
    // (ie. it is what should be used for further lookups. Example:
    //  although PointParent has the PASSTHRU, the actual point object
    //  is what needs to be worked on. It is the true target.)
    // So, push it onto the stack so it will be used by the code-proto's
    // stack-mapped 'parent' slot.

    // However, if it is a code-proto, then we push it's parent instead
    // and if it is a code-proto for a block, then we must go up the
    // context reference and look from there...
    // (Since it already uses the stack-parent to reference it's parent 
    //  context, but we don't want to refer to that context if
    //  we start a new activation/passthru (which is why we're here) 
    //  we look up for it's parent)
    // *NOTE* code-proto's always have the PROTO_REF_CODE, even for the
    // cloned block code-proto's.

    // first we preserve the fp. This is because as we follow
    // context references, the next lookups of the parent (as a
    // stack relative) from within a code-proto, they will need
    // a new frame of reference, and everybody uses the Cpu
    // global
    fp = Cpu->fp;
    while (1) {
      if (PROTO_REFTYPE(target) == PROTO_REF_CODE) {
        objectGetSlot(target, ATOMparent, &target);
      }
      if (PROTO_REFTYPE(target) == PROTO_REF_CONTEXT) {
        Cpu->fp = ((uint32)target & PROTO_REF_MASK) >> 1;
	// Get the code-proto/block that was pointed at in that context
	target  = (Proto) getFPRelativeAt(Cpu->fp, 2);
	continue;
      }
      break;
    }
    // Restore the fp
    Cpu->fp = fp;
    push(Cpu, (uint32) target);

    // Get the codeProto via the data in the passThru
    codeProto = protoSlotData(object, slotPtr);

    // Now get the '_code_' blob from the Proto pointed at in slotPtr
    objectGetSlot(codeProto, ATOM_code_, &obj);
    p = (char *) objectPointerValue(obj);
    // Push the code-proto to the stack
    push(Cpu, (uint32) codeProto);
    
    // Set the cpu to the first part of that code
    Cpu->pc = (uint32) p;

    // The code will then continue on from here, but the 'Call'
    // instruction is now complete.

    // That is, the VM will continue running bytecodes (which is 
    // what called this routine anyways), but a new activation
    // record has been created
}


// actionCode - not implemented yet.
//
inline void actionCode(Proto object, ProtoSlot slotPtr)
{
    Proto obj;
    char * p;
    Proto codeProto;
    
    
    // A marker to me that we need to implement this
    assert(0);
    // The target of the message is the code proto. However, the code
    // called in this fashion is like a 'block'. Therefore it needs to
    // have a parent. It should get the parent relative to the frame
    // pointer. The method that called this block, preserved it's 'self'
    // there.
    push(Cpu, getFrameAt(Cpu,-2));

    // Get the codeProto via the data in the passThru
    codeProto = protoSlotData(object, slotPtr);

    // Now get the '_code_' from the Proto pointed at in slotPtr
    objectGetSlot(codeProto, ATOM_code_, &obj);
    p = (char *) objectPointerValue(obj);
    
    // Set the cpu to the first part of that code
    Cpu->pc = (uint32) p;

    // The code will then continue on from here, but the 'Call'
    // instruction is now complete.

    // That is, the VM will continue running bytecodes (which is 
    // what called this routine anyways), but a new activation
    // record has been created
    //// Generate an exception for now
    assert(0);
}



// actionIntrinsic calls the code via C for the argument in the slot
//

inline void actionIntrinsic (Proto object, ProtoSlot slotPtr)
{
    uint32 obj;
    void (*func)(void);
    
    
    // Ok, get the data for the object
    obj = (uint32) protoSlotData(object, slotPtr);

    func =  ( void (*)(void) ) (obj);
    

    (*func)();			// Call the function
    
    // The intrinsic takes care of the return
    
}




//////////////////////////////////////////////////////////////
//
//
// I N T R I N S I C S
//
//
//
//////////////////////////////////////////////////////////////


// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple boolean equality function
void intrinsicTrueEqual(void)
{
    int i;
    Proto arg;
    Proto proto;
    Proto bool;

    arg       = (Proto) stackAt(Cpu,4);

    bool  = objectResolveForwarders(TrueObj);
    proto = objectResolveForwarders(arg);

    if (bool == proto) {
	i = 1;
    } else {
	i = 0;
    }
    
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}


// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple boolean inequality function
void intrinsicTrueNotEqual(void)
{
    int i;
    Proto arg;
    Proto proto;
    Proto bool;

    arg   = (Proto) stackAt(Cpu,4);

    bool  = objectResolveForwarders(TrueObj);
    proto = objectResolveForwarders(arg);

    if (bool != proto) {
	i = 1;
    } else {
	i = 0;
    }
    
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple boolean equality function
void intrinsicFalseEqual(void)
{
    int i;
    Proto arg;
    Proto proto;
    Proto bool;

    arg       = (Proto) stackAt(Cpu,4);

    bool  = objectResolveForwarders(FalseObj);
    proto = objectResolveForwarders(arg);

    if (bool == proto) {
	i = 1;
    } else {
	i = 0;
    }
    
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}


// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple boolean inequality function
void intrinsicFalseNotEqual(void)
{
    int i;
    Proto arg;
    Proto proto;
    Proto bool;

    arg   = (Proto) stackAt(Cpu,4);

    bool  = objectResolveForwarders(FalseObj);
    proto = objectResolveForwarders(arg);

    if (bool != proto) {
	i = 1;
    } else {
	i = 0;
    }
    
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}


// Arguments: target, 'setSlot:With:', slotname/atom, new value
// Return: target
// Notes:
// If the slot doesn't exist, it goes up the parent chain
// to find a non-code proto to set the slot in
//
// This is a tricky method. If the slot exists, it will set the slot
// to a new value. This doesn't matter if the slot is a setter or a
// getter.
// Now if a slot doesn't exist, the code will look up the parent
// chain for a writable slot. Once it finds one it will create
// the slot or slots. If the name has a colon on the end, it will
// create a setter. If the name doesn't have a colon, then it will
// be just a getter
// *Note* this implies that a local variable slot cannot be created
// at runtime with this method
//
void intrinsicSetSlotWith (void)
{
    Proto      proto;
    Proto      initialProto;
    Proto      slotNameObj;
    Atom       slotName;
    uint32     newData;
    uint32     initialFP;
    ProtoSlot  slotPtr;
    char       *p;
    uint32     i;
    char       buff[128];
    
    
    
    
    initialProto = proto = (Proto) stackAt(Cpu, 2);
    slotNameObj  = (Proto)   stackAt(Cpu, 4);
    newData      = (uint32) stackAt(Cpu, 5);

    // Preserve the FP
    initialFP = Cpu->fp;
    
    
    // Handle Atom's or Strings
    slotName = getAtom(slotNameObj);
    
    
    while (1) {

	
	proto = objectResolveForwarders(proto);

	// Try to find the slot
	if (objectLookupKey(proto, (uint32) slotName, &slotPtr)) {
	    // The key was found

	    // Set the data
	    protoSlotData(proto, slotPtr) = newData;
	    
	    // Now return the object
	    VMReturn(Cpu, (unsigned int) initialProto, 5);
	    // Restore the FP
	    Cpu->fp = initialFP;
	    return;
	}
		
	// If not there, see if this is a code proto
        if ( ((uint32) proto & PROTO_REF_TYPE) == PROTO_REF_CODE ) {
	    // Lookup parent
	    // Look for the parent
	    i = objectLookupKey(proto, (uint32) ATOMparent, &slotPtr);
	    
	    // If there is a parent
	    if (i) {
		objectGetSlot(proto, ATOMparent, &proto);
	        if (PROTO_REFTYPE(proto) == PROTO_REF_CONTEXT ) {
		  // We need to handle this case
		  assert(0);
		}
		continue;
	    }
	    // If no parent, this is a problem
	    else {
		//// Need to generate an exception
		sprintf(buff, "got orphaned when looking for a writable proto\n");
		printString(buff);
		// Restore the FP
		Cpu->fp = initialFP;
		exit(2);
	    }
	} else {
	    // Check if the slotName has a ':'
	    p = strchr(atomToString(slotName), ':');
	    
	    // If there is a colon
	    if (p) {
		p++;
		if (*p) {
		    //// Need to generate an exception
		    sprintf(buff, "too many colons for a setSlotWith [%s]\n", atomToString(slotName));
		    printString(buff);
		    // Restore the FP
		    Cpu->fp = initialFP;
		    exit(2);
		}
		// Create a getter
		objectSetSlot(proto, slotName, PROTO_ACTION_SET, (uint32) newData);

		// Create a setter
		strcpy(buff, atomToString(slotName));
		p = strchr(buff, ':');
		*p = 0;
		objectSetSlotSame(proto, stringToAtom(buff), slotName, PROTO_ACTION_GET);
	    } else {
		// Create a getter
		objectSetSlot(proto, slotName, PROTO_ACTION_GET, (uint32) newData);
	    }
	    
	    // Now return the object
	    VMReturn(Cpu, (unsigned int) initialProto, 5);
	    // Restore the FP
	    Cpu->fp = initialFP;
	    return;
	} // if code proto
    } // while 
    
}

// Arguments: target 'slotNames'
// Return: Array of the slotNames
// Notes:
//
void intrinsicSlotNames (void)
{
    Proto      proto;
    Proto      resultObject;
    ProtoSlot  slotPtr;
    int        j;
    
    
    proto = (Proto) stackAt(Cpu, 2);

    proto = objectResolveForwarders(proto);

    GCOFF();
    
    resultObject = createArray(nil);
    

    j = 0;
    while (1) {
	slotPtr = objectEnumerate(proto, &j, 0);
	if (slotPtr == nil) {
	    break;
	}

	addToArray(resultObject, (Proto) slotKey(slotPtr));
    }

    GCON();
    
    VMReturn(Cpu, (unsigned int) resultObject, 3);
    
}

// Arguments: target 'hasSlot:' slotName
// Return: boolean
// Notes:
//
void intrinsicHasSlot (void)
{
    Proto      proto;
    Proto      arg;
    Proto      resultObject;
    Proto      tmp;
    int        i;
    Atom       key;
    
    
    proto = (Proto) stackAt(Cpu, 2);
    arg   = (Proto) stackAt(Cpu, 4);

    key = getAtom(arg);
    
    i = objectGetSlot(proto, key, &tmp);

    if (i) {
	resultObject = TrueObj;
    } else {
	resultObject = FalseObj;
    }
    
    VMReturn(Cpu, (unsigned int) resultObject, 4);
}

// Arguments: target 'getSlot:' slotName
// Return: slot contents
// Notes:
//
void intrinsicGetSlot (void)
{
    Proto      proto;
    Proto      arg;
    Proto      resultObject;
    Proto      tmp;
    int        i;
    Atom       key;
    
    
    proto = (Proto) stackAt(Cpu, 2);
    arg   = (Proto) stackAt(Cpu, 4);

    key = getAtom(arg);
    
    i = objectGetSlot(proto, key, &tmp);

    if (i) {
	resultObject = tmp;
    } else {
	//// Throw an exception some day
	//// Or have a special doesn't exist object :-)
	resultObject = FalseObj;
    }
    
    VMReturn(Cpu, (unsigned int) resultObject, 4);
}

// Arguments: target 'removeSlot:' slotName
// Return: target
// Notes:
// Remove the slot

void intrinsicRemoveSlot (void)
{
    Proto      proto;
    Proto      arg;
    Atom       key;
    
    
    proto = (Proto) stackAt(Cpu, 2);
    arg   = (Proto) stackAt(Cpu, 4);

    key = getAtom(arg);
    
    objectDeleteSlot(proto, key);

    VMReturn(Cpu, (unsigned int) proto, 4);
}


// Arguments: target 'perform:' slotName
// Return: slot or method's contents
// Notes:
//
void intrinsicPerform (void)
{
    uint32     target;
    uint32     argCount;
    uint32     progCounter;
    
    // Shift the stack up
    target      = (uint32) stackAt(Cpu, 2);
    argCount    = (uint32) stackAt(Cpu, 1);
    progCounter = (uint32) stackAt(Cpu, 0);

    //Decrement the frame's arg count
    argCount = objectIntegerValue((Proto)argCount);
    argCount--;
    argCount = (uint32) objectNewInteger(argCount);
    
    setStackAt(Cpu, 3, target);
    setStackAt(Cpu, 2, argCount);
    setStackAt(Cpu, 1, progCounter);

    // Pop an item off of the stack
    pull(Cpu);
    
    activateSlot();
    
}


// Arguments: target 'traceOn/traceOff'
// Return: target
// Notes:
// This turns message tracing On or Off
void intrinsicTrace (void)
{
    Proto target;
    Atom mesg;

    target    = (Proto) stackAt(Cpu,3);
    mesg      = (Atom) stackAt(Cpu,3);

    if (mesg == stringToAtom("traceOff")) {
	runtimeActivateTrace = 0;
    } else {
	runtimeActivateTrace = 1;
    }

    VMReturn(Cpu, (unsigned int) target, 4);
}

// Arguments: target 'isNil'
// Return: true/false
void intrinsicObjectIsNil(void)
{
    if ((Proto)stackAt(Cpu,2) == nil) {
	VMReturn(Cpu, (unsigned int) TrueObj, 3);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 3);
    }
    
}

// Arguments: target 'clone'
// Return: new clone
void intrinsicClone(void)
{
    Proto proto;

    proto = objectClone((Proto)stackAt(Cpu,2));
    
    VMReturn(Cpu, (unsigned int) proto, 3);
}


// Arguments: target <someMessage>
// Return: new clone
// Notes:
// Assumes some binary message, just returns the target, the self
void intrinsicSelf(void)
{
    VMReturn(Cpu, stackAt(Cpu, 2), 3);
}


// Arguments: target '+' argument
// Return: newValue
// Notes:
// Simple add function - imagine the hardware to implement this
// routine via the VM versus a simple adder circuit
void intrinsicIntegerAdd(void)
{
    int i;
    Proto newValue;

    i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) +  // target
        objectIntegerValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}

// Arguments: target '-' argument
// Return: newValue
// Notes:
// Simple integer subtract function
void intrinsicIntegerSubtract(void)
{
    int i;
    Proto newValue;

    i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) -  // target
        objectIntegerValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}

// Arguments: target '*' argument
// Return: newValue
// Notes:
// Simple integer multiply function
void intrinsicIntegerMultiply(void)
{
    int i;
    Proto newValue;

    i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) *  // target
        objectIntegerValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}

// Arguments: target '/' argument
// Return: newValue
// Notes:
// Simple integer divide function
void intrinsicIntegerDivide(void)
{
    int i;
    Proto newValue;

    i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) /  // target
        objectIntegerValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}

// Arguments: target '%' argument
// Return: newValue
// Notes:
// Simple integer modulo function
void intrinsicIntegerModulo(void)
{
    int i;
    Proto newValue;

    i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) %  // target
        objectIntegerValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}


// Arguments: target 'abs'
// Return: newValue
// Notes:
// Simple integer absolute value function
void intrinsicIntegerAbsolute(void)
{
    int i;
    Proto newValue;

    i = objectIntegerValue( (Proto)stackAt(Cpu,2) );  // target
    i = abs(i);
    newValue = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) newValue, 3);
}


// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple integer equality function
void intrinsicIntegerEqual (void)
{
    int i;

    i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) ==  // target
        objectIntegerValue( (Proto)stackAt(Cpu,4) );    // argument
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}


// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple integer inequality function
void intrinsicIntegerNotEqual (void)
{
    int i;

    i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) !=  // target
        objectIntegerValue( (Proto)stackAt(Cpu,4) );    // argument
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target '>' argument
// Return: true/false
// Notes:
// Simple integer greater-than function
void intrinsicIntegerGreater(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_INT) {
	i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) >  // target
	    objectIntegerValue( arg );    // argument
    } else {
	i = 0;
    }

    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target '>=' argument
// Return: true/false
// Notes:
// Simple integer greater-than or equal function
void intrinsicIntegerGreaterEqual(void)
{
    int i;
    Proto arg;
    int left, right;
    

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_INT) {
	left  = objectIntegerValue( (Proto)stackAt(Cpu,2) );
	right = objectIntegerValue( arg );
	i = left >= right;
    } else {
	i = 0;
    }

    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }

}

// Arguments: target '<' argument
// Return: true/false
// Notes:
// Simple integer less-than function
void intrinsicIntegerLess(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_INT) {
	i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) < // target
	    objectIntegerValue( arg );    // argument
    } else {
	i = 1;
    }

    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }

}


// Arguments: target '<=' argument
// Return: true/false
// Notes:
// Simple integer less-than or equal function
void intrinsicIntegerLessEqual(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_INT) {
	i = objectIntegerValue( (Proto)stackAt(Cpu,2) ) <= // target
	    objectIntegerValue( arg );    // argument
    } else {
	i = 1;
    }

    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}


// Arguments: target '@' argument
// Return: newValue
// Notes:
// Simple point create function 
void intrinsicPointCreate(void)
{
    Proto x, y;
    Proto point;
    Proto newPoint;
    ProtoSlot slotPtr;

    x = (Proto) stackAt(Cpu,2);
    y = (Proto) stackAt(Cpu,4);

    objectGetSlot(Globals, stringToAtom("Point"), &point);

    newPoint = objectClone(point);

    if (objectLookupKey(newPoint, (uint32) stringToAtom("x"), &slotPtr)) {
      protoSlotData(newPoint, slotPtr) = x;
    }
    if (objectLookupKey(newPoint, (uint32) stringToAtom("y"), &slotPtr)) {
      protoSlotData(newPoint, slotPtr) = y;
    }
		
    VMReturn(Cpu, (unsigned int) newPoint, 4);
}


// Arguments: target 'print'
// Return: target
// Notes:
// Prints to the default string print routine
void intrinsicIntegerPrint(void)
{
    char buff[8];

    sprintf(buff, "%ld", objectIntegerValue((Proto)stackAt(Cpu,2)) );
    printString(buff);
    VMReturn(Cpu, (unsigned int) stackAt(Cpu, 2), 3);
}

// Arguments: target 'asFloat'
// Return: new float
// Notes:
// This routine tries to return the argument as a float
// Otherwise it returns a nil
void intrinsicIntegerAsFloat (void)
{
    Proto target;
    int i;
    double f;
    Proto result;
    
    
    target    = (Proto) stackAt(Cpu,2);

    i = objectIntegerValue(target);
    f = i;
    
    result = objectNewFloat(f);
    
    VMReturn(Cpu, (unsigned int) result, 3);
}

// Arguments: target 'asString'
// Return: target
// Notes:
// This also handles 'asStringWithFormat' 
void intrinsicIntegerAsString(void)
{
    char fmtbuff[8];
    char buff[64];
    Proto result;
    Atom mesg;
    char * format;
    Atom arg;
    int   returnCount;
    

    mesg      = (Atom) stackAt(Cpu,3);
    if (mesg == stringToAtom("asString")) {
	returnCount = 3;
	format = "%ld";
    } else {
	returnCount = 4;
	arg  = (Atom) stackAt(Cpu,4);
	fmtbuff[0] = '%'; fmtbuff[1] = 0;
	strcat(fmtbuff, atomToString(arg));
	strcat(fmtbuff, "ld");
	format = fmtbuff;
	
	//// Do some better checking here for size DRUDRU
    }
    

    sprintf(buff, format, objectIntegerValue((Proto)stackAt(Cpu,2)) );
    result = createString(buff, strlen(buff));
    VMReturn(Cpu, (unsigned int) result, returnCount);
}



// 
// 
// FLOAT
// 
// 

// Arguments: target
// Return: newValue
// Notes:
// Simple proto copy
void intrinsicFloatCopy(void)
{
    double f;
    Proto newValue;

    f = objectFloatValue( (Proto)stackAt(Cpu,2) );  // target
    newValue = objectNewFloat(f);
    VMReturn(Cpu, (unsigned int) newValue, 3);
}

// Arguments: target '+' argument
// Return: newValue
// Notes:
// Simple add function - imagine the hardware to implement this
// routine via the VM versus a simple adder circuit
void intrinsicFloatAdd(void)
{
    double f;
    Proto newValue;

    f = objectFloatValue( (Proto)stackAt(Cpu,2) ) +  // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewFloat(f);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}

// Arguments: target '-' argument
// Return: newValue
// Notes:
// Simple integer subtract function
void intrinsicFloatSubtract(void)
{
    double f;
    Proto newValue;

    f = objectFloatValue( (Proto)stackAt(Cpu,2) ) -  // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewFloat(f);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}

// Arguments: target '*' argument
// Return: newValue
// Notes:
// Simple integer multiply function
void intrinsicFloatMultiply(void)
{
    double f;
    Proto newValue;

    f = objectFloatValue( (Proto)stackAt(Cpu,2) ) *  // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewFloat(f);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}

// Arguments: target '/' argument
// Return: newValue
// Notes:
// Simple integer divide function
void intrinsicFloatDivide(void)
{
    double f;
    Proto newValue;

    f = objectFloatValue( (Proto)stackAt(Cpu,2) ) /  // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );   // argument
    newValue = objectNewFloat(f);
    VMReturn(Cpu, (unsigned int) newValue, 4);
}

// Arguments: target 'abs'
// Return: newValue
// Notes:
// Simple integer absolute value function
void intrinsicFloatAbsolute(void)
{
    double f;
    Proto newValue;

    f = objectFloatValue( (Proto)stackAt(Cpu,2) );  // target
    f = abs(f);
    newValue = objectNewFloat(f);
    VMReturn(Cpu, (unsigned int) newValue, 3);
}


// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple integer equality function
void intrinsicFloatEqual (void)
{
    int i;

    i = objectFloatValue( (Proto)stackAt(Cpu,2) ) ==  // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );    // argument
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}


// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple integer inequality function
void intrinsicFloatNotEqual (void)
{
    int i;

    i = objectFloatValue( (Proto)stackAt(Cpu,2) ) !=  // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );    // argument
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target '>' argument
// Return: true/false
// Notes:
// Simple integer greater-than function
void intrinsicFloatGreater(void)
{
    int i;

    i = objectFloatValue( (Proto)stackAt(Cpu,2) ) >   // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );    // argument
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target '>=' argument
// Return: true/false
// Notes:
// Simple integer greater-than or equal function
void intrinsicFloatGreaterEqual(void)
{
    int i;

    i = objectFloatValue( (Proto)stackAt(Cpu,2) ) >=   // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );    // argument
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target '<' argument
// Return: true/false
// Notes:
// Simple integer less-than function
void intrinsicFloatLess(void)
{
    int i;

    i = objectFloatValue( (Proto)stackAt(Cpu,2) ) <   // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );    // argument
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}


// Arguments: target '<=' argument
// Return: true/false
// Notes:
// Simple integer less-than or equal function
void intrinsicFloatLessEqual(void)
{
    int i;
    
    i = objectFloatValue( (Proto)stackAt(Cpu,2) ) <=  // target
        objectFloatValue( (Proto)stackAt(Cpu,4) );    // argument
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}



// Arguments: target 'print'
// Return: target
// Notes:
// Prints to the default string print routine
void intrinsicFloatPrint(void)
{
    char buff[32];

    sprintf(buff, "%f", objectFloatValue((Proto)stackAt(Cpu,2)) );
    printString(buff);
    VMReturn(Cpu, (unsigned int) stackAt(Cpu, 2), 3);
}

// Arguments: target 'asString'
// Return: target
// Notes:
// This also handles 'asStringWithFormat' 
void intrinsicFloatAsString(void)
{
    char fmtbuff[8];
    char buff[64];
    Proto result;
    Atom mesg;
    char * format;
    Atom arg;
    int   returnCount;
    

    mesg      = (Atom) stackAt(Cpu,3);
    if (mesg == stringToAtom("asString")) {
	returnCount = 3;
	format = "%lf";
    } else {
	returnCount = 4;
	arg  = (Atom) stackAt(Cpu,4);
	fmtbuff[0] = '%'; fmtbuff[1] = 0;
	strcat(fmtbuff, atomToString(arg));
	strcat(fmtbuff, "lf");
	format = fmtbuff;
	
	//// Do some better checking here for size DRUDRU
    }
    

    sprintf(buff, format, objectFloatValue((Proto)stackAt(Cpu,2)) );
    result = createString(buff, strlen(buff));
    VMReturn(Cpu, (unsigned int) result, returnCount);
}


// Arguments: target 'asInteger'
// Return: target
// Notes:
void intrinsicFloatAsInteger(void)
{
    double f;
    Proto newValue;

    f = objectFloatValue( (Proto)stackAt(Cpu,2) );  // target
    newValue = objectNewInteger( (int) f);
    VMReturn(Cpu, (unsigned int) newValue, 3);
}


// Arguments: target 'asIntegerScaled:'
// Return: target as an integer after being multiplied by an integer scalar
// Notes:
void intrinsicFloatAsIntegerScaled(void)
{
    double f;
    int i;
    Proto newValue;

    f = objectFloatValue( (Proto)stackAt(Cpu,2) );  // target
    i = f * objectIntegerValue( (Proto)stackAt(Cpu,4) );    // argument

    newValue = objectNewInteger( i );
    VMReturn(Cpu, (unsigned int) newValue, 4);
}




// Arguments: target 'import:' argument
// Return: target
// Notes:
// Loads in that Kit
void intrinsicKitImport (void)
{
    char buff[80];
    Atom  kitName;
    Proto kit;
    Proto root;
    Proto start;
    Proto target;
    Proto imports;
    Proto tmpObj;
    uint32 i;
    uint32 j;
    ProtoSlot slotPtr;
    

    GCOFF();

    kitName = (Atom)stackAt(Cpu,4);
    
    // sprintf(buff, "\nImport: %s\n", atomToString(kitName) );
    // printString(buff);

    // See if it was already imported
    objectGetSlot(Kit, stringToAtom("imported"), &imports);
    
    i = objectGetSlot(imports, kitName, &tmpObj);
    if (i) {
	// This kit was already imported
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
        GCON();
	return;
    } else {
	objectSetSlot(imports, kitName, PROTO_ACTION_GET, (uint32) TrueObj);
    }
    
    

    // Load the module into the system and call it's start
    // method

    // Step 1
    // Load the module into memory
    kit = intrinsicLoadModule(kitName);

    if (nil == kit) {
	sprintf(buff, "kit couldn't load [%s].Exiting.\n", atomToString(kitName) );
	printString(buff);
	exit(1);
    }

    //// Do wonderful module checking here ////

    // Ok find the root object
    objectGetSlot(kit, stringToAtom("Root"), &root);

    // Need to assemble everything...
    assembleAll(root);


    // Ok, we were called within the context of some other object.
    // We need to setup a new context with a parent local set to
    // the Root that we have here, the new tree of code that
    // we just imported. That way, when the passThru is called on 
    // Start, it will access that tree instead of from the context
    // that 'import:' was called from.


    // Resolve the forwarders 
    target = objectResolveForwarders(root);
	
    // To create a new context, we just call the actionPassthru
    // routine with a predetermined parent local. This routine
    // does the right thing
    // 
    j = objectLookupKey(target, (uint32) stringToAtom("Start:"), &slotPtr);
    objectGetSlot(root, stringToAtom("Start:"), &start);

    GCON();
    
    actionPassthru(target, slotPtr, target);


    //-----------------------Below this is questionable code---------------

    // OK, we are already in some code, now we need to call some
    // Usually, an intrinsic doesn't call anymore code, but we need to call
    // the '_Start' method in the module
    //
    // So, we need to make a new activation so that the return 
    // goes to the activation that called the 'import'
    //
    
    // LUCKILY, we have done nothing to the stack or processor,
    // all we really need to do is perform the actionPassthru action
    // on the data
    
    // Our context is this... 
    // activateSlot was called...
    // the slot to activate was an intrinsic...
    // we were called inside the intrinsic...
    // the program counter and arguments are on the stack...
    // 
    // we should be able to do a call in such a way that 
    // the return of this will cause the code to flow normally...
    // we need to create a new activation so that the return address
    // is the current PC?
}

Proto intrinsicLoadModule (Atom fileName)
{
    int fd;
    struct stat sb;
    
    int i;
    char * p;
    char buff[128];
    
    unsigned int len;
    Proto obj;
    Proto tmpRoot;
    Proto tmpSpecial;
    int pos;
    
    
    if (SearchPaths[0] == "") {
	pos = 1;
    } else {
	pos = 0;
    }
    
    obj = nil;
    
    while (1) {
	if (SearchPaths[pos] == "") {
	    break;
	}
	
	sprintf(buff,"%s/Compiled/%s.celmod", SearchPaths[pos], atomToString(fileName));
    
	fd = open(buff,O_RDONLY, 0);
	
	if (fd < 0) {
	    pos++;
	    continue;
	}
	
	stat(buff, &sb);
	len = sb.st_size;
	
	p = celmalloc(len);
	i = read(fd, p, len);
	close(fd);
	
	obj = sd2cel(p, len);
	
	objectGetSlot(obj, stringToAtom("Root"), &tmpRoot);

	sprintf(buff,"%sParent", atomToString(fileName));

	// See if there is a 'filename'Parent slot
	if (objectGetSlot(tmpRoot, stringToAtom(buff), &tmpSpecial)) {
	    objectSetSlot(tmpSpecial, stringToAtom("_kitPath"), PROTO_ACTION_GET, (uint32) stringToAtom(SearchPaths[pos]));
	} 
	else
	if (objectGetSlot(tmpRoot, fileName, &tmpSpecial)) {
	    objectSetSlot(tmpSpecial, stringToAtom("_kitPath"), PROTO_ACTION_GET, (uint32) stringToAtom(SearchPaths[pos]));
	}
	
	celfree(p);

	break;
	
    }
    
    return obj;
}

// Arguments: target 'importLib:' object
// Return: target
// Notes:
// Loads in that Kit's library
void intrinsicKitImportLib (void)
{
    Proto kit;
    Proto kitName;
    Proto libraries;
    uint32 i;
    char * * slotNames;
    char * function;
    Proto slotName;
#if 0
    Proto kitPath;
    
    void * handle;
    char * error;
    char buff[128];
#endif


#if 1
    // Get the kit
    kit = (Proto)stackAt(Cpu,4);
    // Get it's name
    assert(objectGetSlot(kit, stringToAtom("name"), &kitName));
    // Notice the change from kit to Kit
    assert(objectGetSlot(Kit, stringToAtom("libraries"), &libraries));

    i = objectGetSlot(libraries, (Atom) kitName, (Proto *)&slotNames);
    if (i == 0) {
       printf("Didn't find kit\n");
       VMReturn(Cpu, (unsigned int) FalseObj, 4);
       return;
    }


    i = 0;
    
    GCOFF();
    while (1) {
	if (slotNames[i][0] == 0) {
	    break;
	}
	
        // The method name is first
	slotName = (Proto) stringToAtom(slotNames[i]);
	i++;
        // The ascii function name is next
	i++;
	function = slotNames[i];
	objectSetSlot(kit, (Atom) slotName, PROTO_ACTION_INTRINSIC, (uint32) function);
	if (slotName == (Proto) stringToAtom("atExit")) {
	   atexit((void (*)(void))function);
	}

	i++;
    }
    GCON();
    
    VMReturn(Cpu, (unsigned int) TrueObj, 4);
#else
    kit = (Proto)stackAt(Cpu,4);
    
    // Get it's path and it's name
    assert(objectGetSlot(kit, stringToAtom("name"), &kitName));
    assert(objectGetSlot(kit, stringToAtom("_kitPath"), &kitPath));

    sprintf(buff, "%s/%s.so", atomToString((Atom)kitPath), atomToString((Atom)kitName));

    handle = dlopen(buff, RTLD_NOW);
    if (!handle) {
	defaultPrintString("Someone botched the location of zap.so");
	defaultPrintString(dlerror());
	exit(5);
    }
    
    slotNames = dlsym(handle, "slotList");
    if ((error = dlerror()) != NULL) {
	defaultPrintString("Couldn't locate 'slotNames' in library.");
	defaultPrintString(error);
	exit(5);
    }

    i = 0;
    
    while (1) {
	if ((*slotNames)[i][0] == 0) {
	    break;
	}
	
	slotName = (Proto) stringToAtom((*slotNames)[i]);
	i++;
	voidoids = dlsym(handle, (*slotNames)[i]);
	if ((error = dlerror()) != NULL) {
	    defaultPrintString("Couldn't locate 'slot intrinisic' in library.");
	    defaultPrintString(error);
	    exit(5);
	}
	    
	objectSetSlot(kit, (Atom) slotName, PROTO_ACTION_INTRINSIC, (uint32) voidoids);

	i++;
    }
    
    VMReturn(Cpu, (unsigned int) TrueObj, 4);
    
#endif

}


// Loads in that Kit's library with just a name
void linkLib (Proto kit, char * name)
{
    Proto libraries;
    uint32 i;
    char * * slotNames;
    char * function;
    Proto slotName;


    // Notice the change from kit to Kit
    assert(objectGetSlot(Kit, stringToAtom("libraries"), &libraries));

    assert(objectGetSlot(libraries, stringToAtom(name), (Proto *)&slotNames));

    i = 0;
    GCOFF();
    while (1) {
	if (slotNames[i][0] == 0) {
	    break;
	}
	
        // The method name is first
	slotName = (Proto) stringToAtom((slotNames)[i]);
	i++;
        // The ascii function name is next
	i++;
	function = slotNames[i];
	objectSetSlot(kit, (Atom) slotName, PROTO_ACTION_INTRINSIC, (uint32) function);

	i++;
    }
    GCON();
}


// Arguments: target 'value'
// Return: NONE - the block does the return
// Notes:
// This changes the thread of control to a block
// **VERY IMPORTANT** Notice how this works for
// value: or value:: as well. The blocks themselves
// will have to check the argCount
void intrinsicValue (void)
{
    Proto block;
    Proto obj;
    char * p;
    
    // The 'target' of this message should be a block.

    // So we need to setup the stack, it needs a frame relative
    // parent slot, which we set to nil (read the comment below)
    // then we set the secret self slot.

    // This code was borrowed heavily from the PASSTHRU action

    // The target should be a block
    block = (Proto)stackAt(Cpu,2);

    // Now get the '_code_' from the block
    objectGetSlot(block, ATOM_code_, &obj);
    p = (char *) objectPointerValue(obj);

    // Push the proper parent - since this is a block, it's
    // parent isn't a GETFRAME slot, it is a CONTEXT slot.
    // Therefore, we will push 'nil' since this data isn't 
    // used
    push(Cpu, (uint32) nil);

    // Push the code-proto to the stack as the SECRET SELF
    push(Cpu, (uint32) block);
    
    // Set the cpu to the first part of that code
    Cpu->pc = (uint32) p;

    // The code will then continue on from here, but the 'Call'
    // instruction is now complete.

    // That is, the VM will continue running bytecodes (which is 
    // what called this routine anyways), but a new activation
    // record has been created
}

// Arguments: target 'whileTrue:' obj
// Return: NONE - the block does the return
// Notes:
// This changes the thread of control to a WhileTrue block
void intrinsicWhileTrue (void)
{
    Proto block;
    Proto obj;
    char * p;

    
    // The 'target' of this message should be a block.

    // So we need to setup the stack, it needs a frame relative
    // parent slot, which we set to nil (read the comment below)
    // then we set the secret self slot.

    // This code was borrowed heavily from the PASSTHRU action

    // The target is the WhileTrueObj
    block = WhileTrueObj;

    // Now get the '_code_' from the block
    objectGetSlot(block, ATOM_code_, &obj);
    p = (char *) objectPointerValue(obj);

    // Push the proper parent - since this is a block, it's
    // parent isn't a GETFRAME slot, it is a CONTEXT slot.
    // Therefore, we will push 'nil' since this data isn't 
    // used
    push(Cpu, (uint32) nil);

    // Push the code-proto to the stack as the SECRET SELF
    push(Cpu, (uint32) block);
    
    // Set the cpu to the first part of that code
    Cpu->pc = (uint32) p;

    // The code will then continue on from here, but the 'Call'
    // instruction is now complete.

    // That is, the VM will continue running bytecodes (which is 
    // what called this routine anyways), but a new activation
    // record has been created
}

// Arguments: target 'whileFalse:' obj
// Return: NONE - the block does the return
// Notes:
// This changes the thread of control to a WhileFalse block
void intrinsicWhileFalse (void)
{
    Proto block;
    Proto obj;
    char * p;

    
    // The 'target' of this message should be a block.

    // So we need to setup the stack, it needs a frame relative
    // parent slot, which we set to nil (read the comment below)
    // then we set the secret self slot.

    // This code was borrowed heavily from the PASSTHRU action

    // The target is the WhileTrueObj
    block = WhileFalseObj;

    // Now get the '_code_' from the block
    objectGetSlot(block, ATOM_code_, &obj);
    p = (char *) objectPointerValue(obj);

    // Push the proper parent - since this is a block, it's
    // parent isn't a GETFRAME slot, it is a CONTEXT slot.
    // Therefore, we will push 'nil' since this data isn't 
    // used
    push(Cpu, (uint32) nil);

    // Push the code-proto to the stack as the SECRET SELF
    push(Cpu, (uint32) block);
    
    // Set the cpu to the first part of that code
    Cpu->pc = (uint32) p;

    // The code will then continue on from here, but the 'Call'
    // instruction is now complete.

    // That is, the VM will continue running bytecodes (which is 
    // what called this routine anyways), but a new activation
    // record has been created
}


// Arguments: target 'print'
// Return: target
// Notes:
// Prints to the default string print routine
void intrinsicAtomPrint(void)
{
    printString(atomToString((Atom)stackAt(Cpu,2)));
    VMReturn(Cpu, (unsigned int) stackAt(Cpu, 2), 3);
}

// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple atom equality function
void intrinsicAtomEqual(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_ATOM) {
	i = ( (Proto)stackAt(Cpu,2) ==  // target
	      (Proto)stackAt(Cpu,4) );    // argument
    } else {
	i = 0;
    }
	
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple atom inequality function
void intrinsicAtomNotEqual(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);          // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_ATOM) {
	i = ( (Proto)stackAt(Cpu,2) !=    // target
	      (Proto)stackAt(Cpu,4) );    // argument
    } else {
	i = 0;
    }
	
    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}


// Arguments: target '>' argument
// Return: true/false
// Notes:
// Simple atom greater-than function
void intrinsicAtomGreater(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_ATOM) {
	i = strcmp(atomToString((Atom) (Proto)stackAt(Cpu,2)), atomToString((Atom)arg));
	i = (i > 0);
    } else {
	i = 1;
    }

    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target '>=' argument
// Return: true/false
// Notes:
// Simple atom greater-than or equal function
void intrinsicAtomGreaterEqual(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_ATOM) {
	i = strcmp(atomToString((Atom) (Proto)stackAt(Cpu,2)), atomToString((Atom)arg));
	i = (i >= 0);
    } else {
	i = 1;
    }

    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }

}

// Arguments: target '<' argument
// Return: true/false
// Notes:
// Simple atom less-than function
void intrinsicAtomLess(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_ATOM) {
	i = strcmp(atomToString((Atom) (Proto)stackAt(Cpu,2)), atomToString((Atom)arg));
	i = (i < 0);
    } else {
	i = 0;
    }

    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }

}


// Arguments: target '<=' argument
// Return: true/false
// Notes:
// Simple atom less-than or equal function
void intrinsicAtomLessEqual(void)
{
    int i;
    Proto arg;

    arg = (Proto)stackAt(Cpu,4);    // argument

    if (PROTO_REFTYPE(arg) == PROTO_REF_ATOM) {
	i = strcmp(atomToString((Atom) (Proto)stackAt(Cpu,2)), atomToString((Atom)arg));
	i = (i <= 0);
    } else {
	i = 0;
    }

    if (i) {
	VMReturn(Cpu, (unsigned int) TrueObj, 4);
    } else {
	VMReturn(Cpu, (unsigned int) FalseObj, 4);
    }
}

// Arguments: target 'size'
// Return: integer size
// Notes:
// Returns the size of the string
void intrinsicAtomSize(void)
{
    int i;
    Proto newValue;

    i = strlen(atomToString((Atom)stackAt(Cpu,2)));
    newValue = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) newValue, 3);
}

// Arguments: target 'asString'
// Return: a new string
// Notes:
// Create a new String object out of this literal
void intrinsicAtomAsString(void)
{
    Proto target;
    int size;
    Proto string;
    Proto newString;
    char * tp;
    
    target = (Proto) stackAt(Cpu,2);

    tp = atomToString((Atom)target);
    size = strlen(tp);


    objectGetSlot(Globals, stringToAtom("String"), &string);

    newString = createString(tp, size);
    
    VMReturn(Cpu, (unsigned int) newString, 3);
}


// Arguments: target 'objectDump'
// Return: junk
void intrinsicObjectDump(void)
{
    objectDump((Proto)stackAt(Cpu,2));
    VMReturn(Cpu, (unsigned int) stackAt(Cpu, 2), 3);
}

// Arguments: target 'objectShallowDump'
// Return: junk
void intrinsicObjectShallowDump(void)
{
    objectShallowDump((Proto)stackAt(Cpu,2));
    VMReturn(Cpu, (unsigned int) stackAt(Cpu, 2), 3);
}


// Arguments: target 'print'
// Return: self/target
// Notes:
// Prints to the default string print routine
void intrinsicStringPrint(void)
{
    Proto target;
    int i;
    Proto blob;
    Proto tmpObj;
    char * p;
    
    
    target = (Proto) stackAt(Cpu,2);

    i = objectGetSlot(target, ATOM_string, &blob);
    
    if (i) {
	i = objectGetSlot(target, ATOMsize, &tmpObj);
	p = objectPointerValue(blob);
	ioOut(p, objectIntegerValue(tmpObj));
    }

    VMReturn(Cpu, (unsigned int) target, 3);
}



// Arguments: target '+' literal/string
// Return: new string that is the combination
// Notes:
// Creates a new string that is the concatenation of the target and the argument
void intrinsicStringAdd(void)
{
    Proto target;
    Proto arg;
    int i;
    int size;
    int argSize;
    Proto blob;
    Proto argBlob;
    Proto string;
    Proto newString;
    Proto tmpObj;
    char * tp;
    char * p;
    char * ap;
    
    
    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);

    tp = NULL;
    size = 0;
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	tp = atomToString((Atom)target);
	size = strlen(tp);
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 0) {
	    tp = "";
	    size = 0;
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, ATOMsize, &tmpObj));
	    size = objectIntegerValue(tmpObj);
	}
    }

    ap = NULL;
    argSize = 0;
    // Normalize the argument
    i = PROTO_REFTYPE(arg);
    if (i == PROTO_REF_ATOM) {
	ap = atomToString((Atom)arg);
	argSize = strlen(ap);
    }
    else if (i == PROTO_REF_PROTO) {
	assert(objectGetSlot(arg, ATOM_string, &argBlob));
	ap = objectPointerValue(argBlob);
	assert(objectGetSlot(arg, ATOMsize, &tmpObj));
	argSize = objectIntegerValue(tmpObj);
    }

    objectGetSlot(Globals, stringToAtom("String"), &string);

    newString = objectClone(string);

    // Create a new blob to hold the strings
    GCOFF();

    blob = objectNewBlob(size + argSize);
    p = objectPointerValue(blob);
    i = objectBlobSize(blob);
    objectSetSlot(newString, ATOM_string, PROTO_ACTION_GET, (uint32) blob);
    objectSetSlot(newString, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size + argSize));
    objectSetSlot(newString, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(i));
    memcpy(p, tp, size);
    memcpy((p+size), ap, argSize);

    GCON();

    VMReturn(Cpu, (unsigned int) newString, 4);
}



// Arguments: target 'at:' position
// Return: the character literal at that position
// Notes:
void intrinsicStringAt(void)
{
    Proto target;
    Proto arg;
    Proto result;
    int i;
    int size;
    char * p;
    char c[2];
    
    
    
    target    = (Proto) stackAt(Cpu,2);
    arg       = (Proto) stackAt(Cpu,4);

    i = PROTO_REFTYPE(arg);

    assert(i == PROTO_REF_INT);

    i = objectIntegerValue(arg);

    
    getStringPtr(target, &p, &size);
    assert(i < size);
    
    c[0] = p[i];
    c[1] = 0;
    
    result = (Proto) stringToAtom(c);
    
    VMReturn(Cpu, (unsigned int) result, 4);

}



// Arguments: target 'insert:' literal/string 'at:' position
// Return: string that is the combination
// Notes:
// Returns the string with that insertion at that position
void intrinsicStringInsertAt(void)
{
    Proto target;
    Proto arg;
    int i;
    int size;
    int argSize;
    int capacity;
    int pos;
    Proto blob;
    Proto argBlob;
    Proto tmp;
    Proto tmpObj;
    char * tp;
    char * p;
    char * ap;
    
    
    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);
    tmp    = (Proto) stackAt(Cpu,5);

    pos = objectIntegerValue(tmp);
    

    ap = NULL;
    argSize = 0;
    
    // Normalize the argument
    i = PROTO_REFTYPE(arg);
    if (i == PROTO_REF_ATOM) {
	ap = atomToString((Atom) arg);
	argSize = strlen(ap);
    }
    else if (i == PROTO_REF_PROTO) {
	assert(objectGetSlot(arg, ATOM_string, &argBlob));
	ap = objectPointerValue(argBlob);
	assert(objectGetSlot(arg, ATOMsize, &tmpObj));
	argSize = objectIntegerValue(tmpObj);
    }

    i = objectGetSlot(target, ATOM_string, &blob);
    
    if (i) {
	assert(objectGetSlot(target, ATOMsize, &tmp));
	size = objectIntegerValue(tmp);
	assert(objectGetSlot(target, ATOMcapacity, &tmp));
	capacity = objectIntegerValue(tmp);

	tp = objectPointerValue(blob);
	
	if ((size + argSize) <= capacity) {
	    // There is room
	    objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size + argSize));
	    memmove(tp+argSize+pos, tp+pos, size-pos);
	    memmove(tp+pos, ap, argSize);
	} else {
	    // There is no room
	    // Create a new blob to hold the strings
	    blob = objectNewBlob(size + argSize);
	    p = (uchar *) objectPointerValue(blob);
	    i = objectBlobSize(blob);
	    objectSetSlot(target, ATOM_string, PROTO_ACTION_GET, (uint32) blob);
	    objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size + argSize));
	    objectSetSlot(target, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(i));
	    memcpy(p, tp, pos);
	    memcpy(p+pos, ap, argSize);
	    memcpy(p+argSize+pos, tp+pos, size-pos);
	}
    } else {
	// Create the blob to hold the string
	blob = objectNewBlob(argSize);
	p = (uchar *) objectPointerValue(blob);
	i = objectBlobSize(blob);
	objectSetSlot(target, ATOM_string, PROTO_ACTION_GET, (uint32) blob);
	objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(argSize));
	objectSetSlot(target, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(i));
	memcpy(p, ap, argSize);
    }
    

    VMReturn(Cpu, (unsigned int) target, 5);
}

// Arguments: target 'removeAt:' position
// Return: string that has the specified position removed
void intrinsicStringRemoveAt(void)
{
    Proto target;
    Proto arg;
    int size;
    int pos;
    char * buff;
    
    
    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);

    pos = objectIntegerValue(arg);
    

    getStringPtr(target, &buff, &size);
    
    assert(pos < size);
    
    objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size - 1));

    memmove(buff+pos, buff+pos+1, size-pos-1);

    VMReturn(Cpu, (unsigned int) target, 4);
}



// Arguments: target 'removeChar:' stringliteral
// Return: self without the characters
// Notes:
// Only first character of string literal is used
// All instances of the character are removed
void intrinsicStringRemoveChar(void)
{
    Proto target;
    Proto arg;
    int size;
    char * p;
    char * buff;
    char * dest;
    char c;
    
    
    
    target    = (Proto) stackAt(Cpu,2);
    arg       = (Proto) stackAt(Cpu,4);

    assert( isStringOrAtom(arg) );
    
    // X is the char to remove
    getStringPtr(arg, &p, &size);
    c = *p;
    
    getStringPtr(target, &buff, &size);
    

    dest = p = buff;
    

    while (1) {
	if (*p != c) {
	    *dest = *p;
	    dest++;
	}
	
	p++;
	
	if ((p - buff) > size) {
	    break;
	}
    }

    size = dest - buff - 1;

    objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size));
    
    VMReturn(Cpu, (unsigned int) target, 4);
}


// Arguments: target 'append:' literal/string
// Return: self
// Notes:
// Just appends a string on to the existing target
void intrinsicStringAppend(void)
{
    Proto target;
    Proto arg;
    int i;
    int size;
    int argSize;
    int capacity;
    Proto blob;
    Proto argBlob;
    Proto tmp;
    Proto tmpObj;
    char * tp;
    char * p;
    char * ap;
    
    
    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);

    ap = NULL;
    argSize = 0;
    
    // Normalize the argument
    i = PROTO_REFTYPE(arg);
    if (i == PROTO_REF_ATOM) {
	ap = atomToString((Atom) arg);
	argSize = strlen(ap);
    }
    else if (i == PROTO_REF_PROTO) {
	assert(objectGetSlot(arg, ATOM_string, &argBlob));
	ap = objectPointerValue(argBlob);
	assert(objectGetSlot(arg, ATOMsize, &tmpObj));
	argSize = objectIntegerValue(tmpObj);
    }

    i = objectGetSlot(target, ATOM_string, &blob);
    
    if (i) {
	assert(objectGetSlot(target, ATOMsize, &tmp));
	size = objectIntegerValue(tmp);
	assert(objectGetSlot(target, ATOMcapacity, &tmp));
	capacity = objectIntegerValue(tmp);

	tp = objectPointerValue(blob);
	
	if ((size + argSize) <= capacity) {
	    // There is room
	    objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size + argSize));
	    memcpy(tp+size, ap, argSize);
	} else {
	    // There is no room
	    // Create a new blob to hold the strings
 	    blob = objectNewBlob(size+argSize);
	    p = (uchar *) objectPointerValue(blob);
	    i = objectBlobSize(blob);
	    objectSetSlot(target, ATOM_string, PROTO_ACTION_GET, (uint32) blob);
	    objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size + argSize));
	    objectSetSlot(target, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(i));
	    memcpy(p, tp, size);
	    memcpy(p+size, ap, argSize);
	}
    } else {
	// Create the blob to hold the string
	blob = objectNewBlob(argSize);
	p = (uchar *) objectPointerValue(blob);
	i = objectBlobSize(blob);
	objectSetSlot(target, ATOM_string, PROTO_ACTION_GET, (uint32) blob);
	objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(argSize));
	objectSetSlot(target, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(i));
	memcpy(p, ap, argSize);
    }
    

    VMReturn(Cpu, (unsigned int) target, 4);

    
}


// Arguments: target 'reverse'
// Return: self
// Notes:
// Reverses the string in place
void intrinsicStringReverse(void)
{
    Proto target;
    int i;
    int size;
    Proto blob;
    Proto tmp;
    char * p;
    char c;
    
    target = (Proto) stackAt(Cpu,2);

    i = objectGetSlot(target, ATOM_string, &blob);
    
    if (i) {
	i = objectGetSlot(target, ATOMsize, &tmp);
	size = objectIntegerValue(tmp);
	if (size > 1) {
	    p = objectPointerValue(blob);
	    
	    // We only need to go through half
	    i = 0;
	    size--;
	    while (1) {
		// Transpose the characters
		c = p[i];
		p[i] = p[size];
		p[size] = c;
		
		i++;
		size--;
		
		if (i >= size) {
		    break;
		}
	    }
	}
    }

    VMReturn(Cpu, (unsigned int) target, 3);
}

// Arguments: target 'lowercase'
// Return: self
// Notes:
// Makes each character lower case in the string
void intrinsicStringLowercase(void)
{    
    Proto target;
    int i;
    Proto blob;
    Proto tmpObj;
    char * p;
    int size;
    
    
    target = (Proto) stackAt(Cpu,2);

    i = objectGetSlot(target, ATOM_string, &blob);
    
    if (i) {
	assert(objectGetSlot(target, ATOMsize, &tmpObj));
	size = objectIntegerValue(tmpObj);
	p = objectPointerValue(blob);
	for (i = 0; i < size; i++) {
	    p[i] = tolower(p[i]);
	}
    }

    VMReturn(Cpu, (unsigned int) target, 3);
}

// Arguments: target 'asAtom'
// Return: variable as an atom
// Notes:
// This routine may create an atom if necessary
void intrinsicStringAsAtom(void)
{
    Proto target;
    Atom  result;
    
    target    = (Proto) stackAt(Cpu,2);
    
    result = getAtom(target);

    VMReturn(Cpu, (unsigned int) result, 3);
}


// Arguments: target 'setCapacity:' integer
// Return: new string that is of that capacity
// Notes:
// Creates a new string that is has that capacity
void intrinsicStringSetCapacity(void)
{
    Proto target;
    Proto arg;
    Proto tmp;
    int i;
    int size;
    int capacity;
    int newCapacity;
    Proto blob;
    char * tp;
    char * p;
    
    
    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);

    newCapacity = objectIntegerValue(arg);

    i = objectGetSlot(target, ATOM_string, &blob);
    
    if (i) {
	assert(objectGetSlot(target, ATOMsize, &tmp));
	size = objectIntegerValue(tmp);
	assert(objectGetSlot(target, ATOMcapacity, &tmp));
	capacity = objectIntegerValue(tmp);

	if (capacity >= newCapacity) {
	    // We're done
	    VMReturn(Cpu, (unsigned int) target, 4);
	}
	
	tp = objectPointerValue(blob);
	
	// There is no room
	// Create a new blob to hold the strings
	blob = objectNewBlob(newCapacity);
	p = (uchar *) objectPointerValue(blob);
	i = objectBlobSize(blob);
	objectSetSlot(target, ATOM_string, PROTO_ACTION_GET, (uint32) blob);
	objectSetSlot(target, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(i));
	memcpy(p, tp, size);
    } else {
	// Create the blob to hold the string
	blob = objectNewBlob(newCapacity);
	i = objectBlobSize(blob);
	objectSetSlot(target, ATOM_string, PROTO_ACTION_GET, (uint32) blob);
	objectSetSlot(target, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(i));
    }
    

    VMReturn(Cpu, (unsigned int) target, 4);
}



char *celStrStr(char *haystack, int haySize, char *needle, int needleSize) 
{
    int i;
    int j;
    

    //// What should I do for a null needle or null haystack?

    i = 0;
    while (1) {
	if (*(needle) == *(haystack+i)) {
	    j = 0;
	    while (1) {
		if (*(needle+j) != *(haystack+i+j)) {
		    break;
		}
		j++;
		if (j == needleSize) {
		    return (haystack + i);
		}
	    }
	}
	
	i++;
	
	if (i > haySize) {
	    break;
	}
    }

    return NULL;
}

char *celRevStrStr(char *haystack, int haySize, char *needle, int needleSize) 
{
    int i;
    int j;
    
    // Need lots of boundary checks here

    i = haySize - needleSize;
    while (1) {
	if (*(needle) == *(haystack+i)) {
	    j = 0;
	    while (1) {
		if (*(needle+j) != *(haystack+i+j)) {
		    break;
		}
		j++;
		if (j == needleSize) {
		    return (haystack + i);
		}
	    }
	}
	
	i--;
	
	if (i < 0) {
	    break;
	}
    }

    return NULL;
}


// Arguments: target 'splitOn:' argument
// Return: new array of strings split by argument
// Notes:
// This routine handles splitOn: and splitOn:atMostCount:
// count: 0 = stop at that number of splits is has that capacity
void intrinsicStringSplit(void)
{
    Proto target;
    Proto arg;
    int i;
    int size = 0;
    Proto blob;
    Proto mesg;
    Proto countObj;
    Proto argBlob;
    int count = 0;
    int returnCount;
    char * needle = "";
    int    needleSize = 0;
    int splitCount;
    int chunkSize;
    char * start;
    Proto tmpObj;
    char * p;
    char * tp = "";
    int  isWithCount;
    Proto result;
    
    
    
    
    target    = (Proto) stackAt(Cpu,2);
    mesg      = (Proto) stackAt(Cpu,3);
    arg       = (Proto) stackAt(Cpu,4);
    countObj  = (Proto) stackAt(Cpu,5);

    
    if ((Atom) mesg == stringToAtom("splitOn:")) {
	isWithCount = 0;
	returnCount = 4;
    } else {
	isWithCount = 1;
	returnCount = 5;
	count = objectIntegerValue(countObj);
    }
    
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	tp = atomToString((Atom)target);
	size = strlen(tp);
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 0) {
	    tp = "";
	    size = 0;
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, ATOMsize, &tmpObj));
	    size = objectIntegerValue(tmpObj);
	}
    }


    // Normalize the argument 
    i = PROTO_REFTYPE(arg);
    if (i == PROTO_REF_ATOM) {
	needle = atomToString((Atom) arg);
	needleSize = strlen(needle);
    }
    else if (i == PROTO_REF_PROTO) {
	assert(objectGetSlot(arg, ATOM_string, &argBlob));
	needle = objectPointerValue(argBlob);
	assert(objectGetSlot(arg, ATOMsize, &tmpObj));
	needleSize = objectIntegerValue(tmpObj);
    }
    
    GCOFF();
    
    // Create the array to hold the result
    result = createArray(nil);


    start = tp;
    
    splitCount = 0;
    
    while (1) {
	
	if ( isWithCount && (count == splitCount) ) {
	    break;
	}
	
	p = celStrStr(start, size, needle, needleSize);
	
	if (p == NULL) {
	    break;
	}
	
	// It was found
	chunkSize = p - start;
	
	addToArray(result, createString(start, chunkSize));
	
	// Skip over the pattern
	start = start + chunkSize + needleSize;
	size = size - chunkSize - needleSize;
	
	splitCount ++;
	
    }
    addToArray(result, createString(start, size));
    
    GCON();
    

    VMReturn(Cpu, (unsigned int) result, returnCount);
}


// Arguments: target 'splitCSV'
// Return: new array of strings delimited in the Comma Separated Values format
// Notes:
void intrinsicStringSplitCSV(void)
{
    Proto target;
    int size;
    char * start;
    char * p;
    char * tp;
    char * last;
    int  inString;
    int  lastWasQuoted;
    Proto result;
    
    
    
    
    target    = (Proto) stackAt(Cpu,2);
    

    getStringPtr(target, &tp, &size);
    
    // Create the array to hold the result
    result = createArray(nil);


    GCOFF();
    

    p = last = start = tp;
    inString = 0;
    lastWasQuoted = 0;
    
    while (1) {
	if (*p == '"') {
	    if (inString) {
		inString = 0;
		lastWasQuoted = 1;
	    } else {
		inString = 1;
		last++;
	    }
	}
	
	if (*p == ',' && !inString) {
	    // It was found
	    addToArray(result, createString(last, (p - last - lastWasQuoted)));
	    
	    // Move the last pointer
	    last = p + 1;
	    
	    // Reset lastWasQuoted
	    lastWasQuoted = 0;
	}
	
	p++;
	
	if ((p - start) >= size) {
	    break;
	}
    }
    addToArray(result, createString(last, (p - last)));

    GCON();

    VMReturn(Cpu, (unsigned int) result, 3);
}


// Copies the string from an Atom or String into the specified buffer
int getString (Proto target, char * buff, int bsize)
{
    Proto blob;
    Proto tmpObj;
    int size;
    int i;
    char * tp;
    
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	tp = atomToString((Atom)target);
	size = strlen(tp);
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 0) {
	    tp = "";
	    size = 0;
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, ATOMsize, &tmpObj));
	    size = objectIntegerValue(tmpObj);
	}
    } else {
	assert(0);
    }

    // Make a copy of the command
    assert(size < bsize);
    memcpy(buff, tp, size);
    buff[size] = 0;

    return 0;
}


// Gets the pointer and size of the data in an Atom or String
int getStringPtr (Proto target, char ** buff, int * bsize)
{
    Proto blob;
    Proto tmpObj;
    int size;
    int i;
    char * tp;
    
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	tp = atomToString((Atom)target);
	size = strlen(tp);
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 0) {
	    tp = "";
	    size = 0;
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, ATOMsize, &tmpObj));
	    size = objectIntegerValue(tmpObj);
	}
    } else {
	assert(0);
    }

    *buff  = tp;
    *bsize = size;

    return 0;
}


// Creates a C-String from the specified Cel String/Atom
// Note, this requires the particular customer to use celfree
// on the string
char * string2CString (Proto target)
{
    int    size;
    char * tp;
    char * buff;
    
    getStringPtr(target, &tp, &size);
    buff = celcalloc(size + 1);
    memcpy(buff, tp, (size+1));
    return buff;
}

    
// Get an atom from an atom or a string
Atom getAtom(Proto target)
{
    Proto blob;
    Proto tmpObj;
    int size;
    int i;
    char * tp;
    Atom   retVal;
    char * buff;
    
    
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	retVal = (Atom) target;
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 0) {
	    retVal = stringToAtom("");
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, ATOMsize, &tmpObj));
	    size = objectIntegerValue(tmpObj);
	    buff = celcalloc(size + 1);
	    memcpy(buff, tp, size);
	    buff[size] = 0;
	    retVal = stringToAtom(buff);
	    celfree(buff);
	}
    } else {
	assert(0);
    }

    return retVal;
}


// Creates a new string that is a copy of the specified arguments
Proto createString (char * buff, int size)
{
    int i;
    Proto blob;
    Proto string;
    Proto newString;

    char * p;

    objectGetSlot(Globals, stringToAtom("String"), &string);

    GCOFF();
    
    newString = objectClone(string);

    // Create a new blob to hold the strings
    blob = objectNewBlob(size);
    p = (uchar *) objectPointerValue(blob);
    i = objectBlobSize(blob);
    objectSetSlot(newString, ATOM_string, PROTO_ACTION_GET, (uint32) blob);
    objectSetSlot(newString, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size));
    objectSetSlot(newString, ATOMcapacity, PROTO_ACTION_GET, (uint32) objectNewInteger(i));
    memcpy(p, buff, size);

    GCON();
    
    return newString;
}


// Sets an existing string with new data
// Avoids creating an object, it copies write into
// the existing string's buffer memory
void setString (Proto str, char * buff, int size)
{
    int i;
    char * p;

    getStringPtr(str, &p, &i);

    //// We should really look at the capacity, since
    //// that is what is really important
    //// expansion is also another issue
    assert(i >= size);

    memcpy(p, buff, size);
    objectSetSlot(str, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size));
}


// Verifies that proto is string or atom
int isStringOrAtom (Proto target)
{
    Proto blob;
    int i;
    
    // Normalize the target
    i = PROTO_REFTYPE(target);

    if (i == PROTO_REF_ATOM) {
	return 1;
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 1) {
	    return (1);
	}
    }
    
    return (0);
}


// Arguments: target 'copyFrom:' integer 'To:' integer
// Return: new string that is the slice of the original
// Notes:
// Currently it doesn't deal with negative offsets, but
// it will
void intrinsicStringSlice(void)
{
    Proto target;
    int i;
    int size;
    Proto blob;
    Proto beginObj;
    Proto endObj;
    int   begin;
    int   end;
    char * start;
    Proto tmpObj;
    char * tp = "";
    Proto result;
    
    
    
    
    target    = (Proto) stackAt(Cpu,2);
    beginObj  = (Proto) stackAt(Cpu,4);
    endObj    = (Proto) stackAt(Cpu,5);

    
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	tp = atomToString((Atom)target);
	size = strlen(tp);
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 0) {
	    tp = "";
	    size = 0;
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, ATOMsize, &tmpObj));
	    size = objectIntegerValue(tmpObj);
	}
    }



    begin = objectIntegerValue(beginObj);
    end   = objectIntegerValue(endObj);
    

    start = tp;

    //// No boundary checks - BAD - DRUDRU
    start += begin;
    
    result = createString(start, (end - begin));
    

    VMReturn(Cpu, (unsigned int) result, 5);
}



// Arguments: target 'asInteger'
// Return: new integer
// Notes:
// This routine tries to return the argument as an integer
// Otherwise it returns a nil
void intrinsicStringAsInteger(void)
{
    Proto target;
    int i;
    int size;
    Proto blob;
    Proto mesg;
    Proto tmpObj;
    char * p;
    char * tp = "";
    Proto result;
    char buff[16];
    
    
    target    = (Proto) stackAt(Cpu,2);
    mesg      = (Proto) stackAt(Cpu,3);
    
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	tp = atomToString((Atom)target);
	size = strlen(tp);
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 0) {
	    tp = "";
	    size = 0;
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, ATOMsize, &tmpObj));
	    size = objectIntegerValue(tmpObj);

	    // Insure that this is a string value terminated with a null
	    assert(size <= 15);
	    memcpy(buff, tp, size);
	    buff[size] = 0;
	    tp = buff;
	}
    }


    if (tp == "") {
	result = nil;
    } else {
	i = strtol(tp, &p, 10);
	if (*p == 0) {
	    result = objectNewInteger(i);
	} else {
	    result = nil;
	}
    }
    
    VMReturn(Cpu, (unsigned int) result, 3);
}

// Arguments: target 'asIntegerScaled:' integer
// Return: new integer
// Notes:
// This routine converts a float to an integer after
// multiplying it by a scalar
void intrinsicStringAsIntegerScaled(void)
{
    Proto target;
    int i;
    int size;
    double f;
    Proto arg;
    char * tp;
    Proto result;
    char buff[22];
    
    
    target    = (Proto) stackAt(Cpu,2);
    arg       = (Proto) stackAt(Cpu,4);
    

    getStringPtr(target, &tp, &size);

    // Insure that this is a string value terminated with a null
    assert(size <= 21);
    memcpy(buff, tp, size);
    buff[size] = 0;
    tp = buff;


    if (tp == "") {
	result = nil;
    } else {
	f = atof(buff);
	i = (int) (f * objectIntegerValue(arg));
	result = objectNewInteger(i);
    }
    
    VMReturn(Cpu, (unsigned int) result, 4);
}


// Arguments: target 'asFloat'
// Return: new integer
// Notes:
// This routine tries to return the argument as a float
// Otherwise it returns a nil
void intrinsicStringAsFloat (void)
{
    Proto target;
    int size;
    double f;
    char * tp;
    Proto result;
    char buff[22];
    
    
    target    = (Proto) stackAt(Cpu,2);

    getStringPtr(target, &tp, &size);

    // Insure that this is a string value terminated with a null
    assert(size <= 21);
    memcpy(buff, tp, size);
    buff[size] = 0;
    tp = buff;


    if (tp == "") {
	result = nil;
    } else {
	f = atof(buff);
	result = objectNewFloat(f);
    }
    
    VMReturn(Cpu, (unsigned int) result, 3);
}


// Arguments: target 'findLast:' argument
// Return: new integer
// Notes:
// This routine tries to find the argument from right to left
// Otherwise it returns a nil
void intrinsicStringFindLast(void)
{
    Proto target;
    int i;
    int size = 0;
    Proto blob;
    Proto mesg;
    Proto tmpObj;
    char * p;
    char * tp = "";
    Proto result;
    Proto arg;
    char * needle = "";
    int    needleSize = 0;
    Proto argBlob;
    
    
    target    = (Proto) stackAt(Cpu,2);
    mesg      = (Proto) stackAt(Cpu,3);
    arg       = (Proto) stackAt(Cpu,4);
    
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	tp = atomToString((Atom)target);
	size = strlen(tp);
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, ATOM_string, &blob);
	if (i == 0) {
	    tp = "";
	    size = 0;
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, ATOMsize, &tmpObj));
	    size = objectIntegerValue(tmpObj);
	}
    }


    // Normalize the argument 
    i = PROTO_REFTYPE(arg);
    if (i == PROTO_REF_ATOM) {
	needle = atomToString((Atom) arg);
	needleSize = strlen(needle);
    }
    else if (i == PROTO_REF_PROTO) {
	assert(objectGetSlot(arg, ATOM_string, &argBlob));
	needle = objectPointerValue(argBlob);
	assert(objectGetSlot(arg, ATOMsize, &tmpObj));
	needleSize = objectIntegerValue(tmpObj);
    }



    p = celRevStrStr(tp, size, needle, needleSize);
    
    if (p == NULL) {
	result = nil;
    } else {
	i = (p - tp);
	result = objectNewInteger(i);
    }
    
    VMReturn(Cpu, (unsigned int) result, 4);
}

// Arguments: target 'urlUnEscape'
// Return: self
// Notes:
// This routine unescapes the characters in a URL
// '+' is changed to a space and % hex hex is changed to that character
void intrinsicStringUrlUnEscape(void)
{
    Proto target;
    int i;
    int x;
    int size;
    char * p;
    char * buff;
    char * dest;
    char c;
    
    
    
    target    = (Proto) stackAt(Cpu,2);

    i = PROTO_REFTYPE(target);

    assert( isStringOrAtom(target) );
    

    
    getStringPtr(target, &buff, &size);
    

    dest = p = buff;
    

    while (1) {
	if (*p == '+') {
	    *dest = ' ';
	} else if (*p == '%') {
	    //// Boundary check could be used here..
	    p++;
	    c = tolower(*p);
	    if (c > '9') {
		x = 16 * (c - 'a' + 10);
	    } else {
		x = 16 * (c - '0');
	    }

	    p++;
	    c = tolower(*p);
	    if (c > '9') {
		x = x + (c - 'a' + 10);
	    } else {
		x = x + (c - '0');
	    }

	    *dest = (unsigned char) x;
	} else {
	    *dest = *p;
	}
	
	dest++;
	p++;
	
	if ((p - buff) > size) {
	    break;
	}
    }

    size = dest - buff - 1;

    objectSetSlot(target, ATOMsize, PROTO_ACTION_GET, (uint32) objectNewInteger(size));
    
    VMReturn(Cpu, (unsigned int) target, 3);
}

// Arguments: target 'urlEscape'
// Return: self
// Notes:
// This routine escapes the characters in a URL
// '+' is changed to a space and % hex hex is changed to that character
//// DRUDRU - for now space is just changed to '+'
void intrinsicStringUrlEscape(void)
{
    Proto target;
    int i;
    int size;
    char * p;
    char * buff;
    
    
    target    = (Proto) stackAt(Cpu,2);

    i = PROTO_REFTYPE(target);

    assert( isStringOrAtom(target) );
    

    
    getStringPtr(target, &buff, &size);
    

    p = buff;
    

    while (1) {
	if (*p == ' ') {
	    *p = '+';
	}

	p++;
	
	if ((p - buff) > size) {
	    break;
	}
    }

    VMReturn(Cpu, (unsigned int) target, 3);
}


// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple string equality function
void intrinsicStringEqual(void)
{
    int i;
    Proto target;
    Proto arg;
    char * targBuff;
    char * argBuff;
    int targSize;
    int argSize;
    

    target = (Proto)stackAt(Cpu,2);    // argument
    arg    = (Proto)stackAt(Cpu,4);    // argument

    if (isStringOrAtom(arg) != 1) {
	//// THROW EXCEPTION
	assert(0);
    }

    getStringPtr(target, &targBuff, &targSize);
    getStringPtr(arg, &argBuff, &argSize);
    
    // Compare sizes, do the memcmp
    if (targSize != argSize) {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
    } else {
	i = memcmp(targBuff, argBuff, targSize);
	if (i == 0) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    }
}

// Arguments: target '>' argument
// Return: true/false
// Notes:
// Simple string greater-than function
void intrinsicStringGreater(void)
{
    int i;
    Proto target;
    Proto arg;
    char * targBuff;
    char * argBuff;
    int targSize;
    int argSize;
    

    target = (Proto)stackAt(Cpu,2);    // argument
    arg    = (Proto)stackAt(Cpu,4);    // argument

    if (isStringOrAtom(arg) != 1) {
	//// THROW EXCEPTION
	assert(0);
    }

    getStringPtr(target, &targBuff, &targSize);
    getStringPtr(arg, &argBuff, &argSize);
    
    // Compare sizes, do the memcmp
    if (targSize != argSize) {
	i = (targSize - argSize);
	if ( i > 0 ) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    } else {
	i = memcmp(targBuff, argBuff, targSize);
	if (i > 0) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    }
}

// Arguments: target '>=' argument
// Return: true/false
// Notes:
// Simple string greater-than or equal function
void intrinsicStringGreaterEqual(void)
{
    int i;
    Proto target;
    Proto arg;
    char * targBuff;
    char * argBuff;
    int targSize;
    int argSize;
    

    target = (Proto)stackAt(Cpu,2);    // argument
    arg    = (Proto)stackAt(Cpu,4);    // argument

    if (isStringOrAtom(arg) != 1) {
	//// THROW EXCEPTION
	assert(0);
    }

    getStringPtr(target, &targBuff, &targSize);
    getStringPtr(arg, &argBuff, &argSize);
    
    // Compare sizes, do the memcmp
    if (targSize != argSize) {
	i = (targSize - argSize);
	if ( i > 0 ) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    } else {
	i = memcmp(targBuff, argBuff, targSize);
	if (i >= 0) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    }
}

// Arguments: target '<' argument
// Return: true/false
// Notes:
// Simple string less-than function
void intrinsicStringLess(void)
{
    int i;
    Proto target;
    Proto arg;
    char * targBuff;
    char * argBuff;
    int targSize;
    int argSize;
    

    target = (Proto)stackAt(Cpu,2);    // argument
    arg    = (Proto)stackAt(Cpu,4);    // argument

    if (isStringOrAtom(arg) != 1) {
	//// THROW EXCEPTION
	assert(0);
    }

    getStringPtr(target, &targBuff, &targSize);
    getStringPtr(arg, &argBuff, &argSize);
    
    // Compare sizes, do the memcmp
    if (targSize != argSize) {
	i = (targSize - argSize);
	if ( i < 0 ) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    } else {
	i = memcmp(targBuff, argBuff, targSize);
	if (i < 0) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    }
}


// Arguments: target '<=' argument
// Return: true/false
// Notes:
// Simple string less-than or equal function
void intrinsicStringLessEqual(void)
{
    int i;
    Proto target;
    Proto arg;
    char * targBuff;
    char * argBuff;
    int targSize;
    int argSize;
    

    target = (Proto)stackAt(Cpu,2);    // argument
    arg    = (Proto)stackAt(Cpu,4);    // argument

    if (isStringOrAtom(arg) != 1) {
	//// THROW EXCEPTION
	assert(0);
    }

    getStringPtr(target, &targBuff, &targSize);
    getStringPtr(arg, &argBuff, &argSize);
    
    // Compare sizes, do the memcmp
    if (targSize != argSize) {
	i = (targSize - argSize);
	if ( i < 0 ) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    } else {
	i = memcmp(targBuff, argBuff, targSize);
	if (i <= 0) {
	    VMReturn(Cpu, (unsigned int) TrueObj, 4);
	} else {
	    VMReturn(Cpu, (unsigned int) FalseObj, 4);
	}
    }
}


// Arguments: target 'new'
// Return: a new array
// Notes:
// Return a new array instance
void intrinsicArrayNew (void)
{
    Proto tmp;

    tmp = createArray(nil);
    
    VMReturn(Cpu, (unsigned int) tmp, 3);
}


// Arguments: target 'new:' <integer size>
// Return: a new array of size
// Notes:
// Return the array position at: x
void intrinsicArrayNewSize (void)
{
    Proto arg;
    int size;
    Proto tmp;

    arg    = (Proto) stackAt(Cpu,4);

    assert(PROTO_REFTYPE(arg) == PROTO_REF_INT);
    
    size = objectIntegerValue(arg);

    tmp = createArraySize(nil, size);
    
    VMReturn(Cpu, (unsigned int) tmp, 4);
}




// Arguments: target 'at:' integer
// Return: value in array
// Notes:
// Return the array position at: x
void intrinsicArrayAt (void)
{
    Proto target;
    Proto arg;
    int i;
    int idx;
    int size;
    Proto tmp;
//    char buff[128];
    
    
    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);

    target = locateObjectWithSlotVia((Proto)ATOMsize, target);

    assert(objectGetSlot(target, ATOMsize, &tmp));
    size = objectIntegerValue(tmp);

    idx = objectIntegerValue(arg);

    //// Throw an exception here
    assert((idx >= 0) && (idx < size));
    
    i = arrayGet(target, idx, &tmp);

//     sprintf(buff, "at: %d  (0x%lX)\n", idx, (uint32) tmp);
//     printString(buff);
    
    VMReturn(Cpu, (unsigned int) tmp, 4);
}

// Arguments: target 'at:otherwiseUse:' integer
// Return: value in array
// Notes:
// Return the array position at: x
void intrinsicArrayAtOtherwiseUse(void)
{
    Proto target;
    Proto arg;
    Proto defaultVal;
    int i;
    int idx;
    int size;
    Proto tmp;
    
    
    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);
    defaultVal = (Proto) stackAt(Cpu,5);

    target = locateObjectWithSlotVia((Proto)ATOMsize, target);

    assert(objectGetSlot(target, ATOMsize, &tmp));
    size = objectIntegerValue(tmp);

    idx = objectIntegerValue(arg);

    // If the index doesn't exist, return this default value
    // the 'otherwise' value
    if ((idx < 0) || (idx >= size)) {
	tmp = defaultVal;
    } else {
	i = arrayGet(target, idx, &tmp);
    }
    
    VMReturn(Cpu, (unsigned int) tmp, 5);
}

// Arguments: target 'at:' integer 'Put:' obj
// Return: self
// Notes:
// Place an object at position
void intrinsicArrayAtPut(void)
{
    Proto target;
    Proto arg;
    int i;
    int idx;
    int size;
    Proto tmp;
//    char buff[128];

    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);

    target = locateObjectWithSlotVia((Proto) ATOM_array, target);

    assert(objectGetSlot(target, ATOMsize, &tmp));
    size = objectIntegerValue(tmp);

    idx = objectIntegerValue(arg);

    //// Throw an exception here
    assert((idx >= 0) && (idx < size));
    
    tmp = (Proto) stackAt(Cpu,5);
    i = arraySet(target, idx, tmp);

//     sprintf(buff, "at: %d  Put: 0x%lX\n", idx, (uint32) tmp);
//     printString(buff);
    
    VMReturn(Cpu, (unsigned int) target, 5);
}

// Arguments: target 'insert:' obj 'at:' integer 
// Return: self
// Notes:
// Insert an object at position
void intrinsicArrayInsertAt(void)
{
    Proto target;
    Proto arg;
    Proto pos;
    int idx;
    Proto tmp;


    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);
    pos    = (Proto) stackAt(Cpu,5);

    target = locateObjectWithSlotVia((Proto) ATOM_array, target);

    assert(objectGetSlot(target, ATOMsize, &tmp));

    idx = objectIntegerValue(pos);

    arrayInsertObjAt(target, arg, idx);

    VMReturn(Cpu, (unsigned int) target, 5);
}


// Arguments: target 'removeAt:' integer
// Return: self
// Notes:
// Remove an object at position and slide the rest to the left
void intrinsicArrayRemoveAt(void)
{
    Proto target;
    Proto arg;
    int idx;
    int size;
    Proto tmp;
    
    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);

    target = locateObjectWithSlotVia((Proto)ATOM_array, target);

    assert(objectGetSlot(target, ATOMsize, &tmp));
    size = objectIntegerValue(tmp);

    idx = objectIntegerValue(arg);

    //// Throw an exception here
    assert((idx >= 0) && (idx < size));
    
    removeFromArray(target, idx);
    
    VMReturn(Cpu, (unsigned int) target, 4);
}


// Arguments: target 'append:' object
// Return: self
// Notes:
// Append an object to the end of the array
void intrinsicArrayAppend(void)
{
    Proto target;
    Proto arg;

    target = (Proto) stackAt(Cpu,2);
    arg    = (Proto) stackAt(Cpu,4);

    target = locateObjectWithSlotVia((Proto)ATOM_array, target);

    addToArray(target, arg);
    
    VMReturn(Cpu, (unsigned int) target, 4);
}




// Arguments: target 'getFromFile:' argument
// Return: new structure or nil
// Notes:
// Loads in that simpledata
void intrinsicSDGetFromFile (void)
{
    Atom  fileName;
    Proto result;
    int fd;
    struct stat sb;
    int len;
    int i;
    Proto obj;
    char * p;
    
    

    fileName = (Atom)stackAt(Cpu,4);
    

    fd = open(atomToString(fileName),O_RDONLY, 0);
	
    if (fd < 0) {
	result = nil;
    } else {
	
	stat(atomToString(fileName), &sb);
	len = sb.st_size;
	
	p = celmalloc(len);
	i = read(fd, p, len);
	assert(i == len);
	
	obj = sd2cel(p, len);
	
	objectSetSlot(obj, ATOMparent, PROTO_ACTION_GET, (uint32) SimpleDataFactory);
	
	result = obj;
	
	celfree(p);
	
	close(fd);
	
    } 

    VMReturn(Cpu, (unsigned int) result, 4);

}


// Arguments: target 'saveToFile:' argument
// Return: target
// Notes:
// 
void intrinsicSDSaveToFile (void)
{
    Proto target;
    Atom fileName;
    void * mem;
    int length;
    char tmpName[256];
    int fd;
    Proto result;
    int i;
    
    
    target   = (Proto) stackAt(Cpu,2);
    fileName = (Atom)stackAt(Cpu,4);

    strcpy(tmpName, atomToString(fileName));
    strcat(tmpName, ".tmp");
    
    
    
    // Resolve the target to it's real proto. 
    // More than likely, we don't need to do
    // this unless the expression was just
    // 'saveToFile: 'somefile''

    mem = cel2sd(target, &length);

    // Now write the file
    fd = open(tmpName,O_CREAT|O_WRONLY|O_TRUNC, 0777);
    if (-1 == fd) {
	result = nil;
    } else {
	
	i = write(fd, mem, length);
	assert(i == length);
	close(fd);
	
	// Atomically rename the tmp file to the SimpleData file
#ifdef WIN32
	i = unlink(atomToString(fileName));
#endif
	i = rename(tmpName, atomToString(fileName));
        if (i != 0) {
	    printf("Error doing rename [errno = %d]\n", errno);
	}
	result = target;
	celfree(mem);
    } 

    VMReturn(Cpu, (unsigned int) result, 4);
}


// Arguments: target 'fromString:' argument
// Return: new structure or nil
// Notes:
// Loads in that simpledata from a string
void intrinsicSDfromString (void)
{
    Proto  strObj;
    Proto result;
    int len;
    Proto obj;
    char * p;
    
    

    strObj = (Proto) stackAt(Cpu,4);

    getStringPtr(strObj, &p, &len);
    
    obj = sd2cel(p, len);
	
    //// DRUDRU this is now gone. If the root object is an array
    //   then this trashes the parent pointer
    //objectSetSlot(obj, Parent, PROTO_ACTION_GET, (uint32) SimpleDataFactory);
	
    result = obj;
	
    VMReturn(Cpu, (unsigned int) result, 4);

}

// Arguments: target 'asString:'
// Return: target
// Notes:
// Converts protos to a SimpleData blob in a string
void intrinsicSDasString (void)
{
    Proto target;
    Proto obj;
    void * mem;
    int length;
    Proto result;
    
    target   = (Proto) stackAt(Cpu,2);
    obj      = (Proto) stackAt(Cpu,4);

    
    // Resolve the target to it's real proto. 
    // More than likely, we don't need to do
    // this unless the expression was just
    // 'saveToFile: 'somefile''

    mem = cel2sd(obj, &length);

    result = createString(mem, length);

    VMReturn(Cpu, (unsigned int) result, 4);
}




// Arguments: target 'run'
// Return: nil
// Notes:
// Forces a collection
void intrinsicGCrun (void)
{

    gcRun();
    VMReturn(Cpu, (unsigned int) nil, 3);

}


// Arguments: target 'isOn'
// Return: nil
// Notes:
// Returns status of gcIsOn variable
void intrinsicGCisOn (void)
{
    int i;
    Proto obj;
    
    i = gcGetIsOn();

    obj = objectNewInteger(i);
    
    VMReturn(Cpu, (unsigned int) obj, 3);
}









//////////////////////////////////////////////////////////////
//
//
// U S E R   S U P P O R T 
//
//
//////////////////////////////////////////////////////////////




void doCelInit (char * module, VM32Cpu * pcpu, int32 noRun)
{
    
    char * stack;
    int stackSize;

    Proto realRoot;
    Proto root;
    Proto start;
    
    Proto tmpObj;

    GCOFF();
    

    // Clear the activate slot timer
#ifdef unix
    timerclear(&lasttv);
#endif
    

    // Setup the memory and the GC
    

    // Setup hooks in our virtual CPU
    pcpu->pushHook     = pushHook;
    pcpu->activateHook = activateSlot;

    if (noRun) {
      pcpu->haltflag = 1; 
    }
      


    // Load the initial program
    realRoot = loadModule(module);
    if (realRoot == nil) {
        printf("Couldn't load module named [%s]\n\n", module);
        exit(1);
    }
	
    


    // Ok find the 'Start'
    objectGetSlot(realRoot, stringToAtom("Root"), &root);
    objectGetSlot(root, stringToAtom("_Start"), &start);

    // Get the stack
    objectGetSlot(start, stringToAtom("Stack"), &tmpObj);
    stackSize = objectIntegerValue(tmpObj);

    stack = (unsigned char *) celcalloc(stackSize);
    
    pcpu->sp = (unsigned int) (stack + stackSize - 4);
    // This stack is the type that points to where
    // the next push will go
    pcpu->stackBottom = (unsigned int) stack;
    pcpu->stackTop = (unsigned int) (stack + stackSize - 4);
    

    // Show the dump of the code (pre assembly)
    //if (debugDump) {
    if (0) {
	objectGetSlot(root, stringToAtom("Start"), &tmpObj);
	objectDump(tmpObj);
    }
    
    // Now assemble the '_Start' slot and all other Slots
    // (At least a 'Start' slot.)
    // This will make 'Start' a passthru to its assembled code proto
    assembleAll(root);


    // Run the Start code by calling 'activateSlot' on the Top with the message
    // '_Start'.
    // This 'primes the pump' with a call to activateSlot.
    // We push the context and then activateSlot, this activates the _Start
    // method. This is a 'passthru'. As such, an activate to it will just
    // setup the FP and Parent pointer (to the target) and then set the
    // PC (Program Counter) to the proper code routine.
    // Then the cpu will have to do more..


    // Push the parent
    push(pcpu, (uint32) root); 
    // Push the code-proto pointer
    push(pcpu, (uint32) root); 
    // Push the preserved FP
    push(pcpu, 0); 
    // Setup the fp
    pcpu->fp = pcpu->sp;

    push(pcpu, (uint32) stringToAtom("_Start"));
    push(pcpu, (uint32) root);
    push(pcpu, 0);		// The Argument count
    push(pcpu, 0);		// The return address


    //if (debugTrace) {
    if (0) {
      // Turn on debug mode
      runtimeActivateTrace = 1;
    }

    GCON();
    
    activateSlot();
}

	   
Proto loadModule (char * fileName)
{
    Proto obj;
    

    obj = intrinsicLoadModule(stringToAtom(fileName));
    

    if (debugDump) { 
      objectDump(obj);
    }

    return obj;
    
}


// checkCompiled - it checks to see if the 
int checkCompiled (char * fileName)
{
    int srcFd;
    int compiledFd;
    struct stat sb;

    unsigned int srcLen;
    unsigned int compLen;
    unsigned int srcTime;
    unsigned int compTime;

    char srcFileName[128];
    char compiledFileName[128];

    int returnVal;
    
    

    strcpy(srcFileName, fileName);
    strcat(srcFileName, ".cel");
    
    strcpy(compiledFileName, "Compiled/");
    strcat(compiledFileName, fileName);
    strcat(compiledFileName, ".celmod");

    
    srcFd = open(srcFileName,O_RDONLY, 0);
    if (srcFd < 0) {
        return -1;
    }
    close(srcFd);
    
    compiledFd = open(compiledFileName,O_RDONLY, 0);
    if (compiledFd < 0) {
        return -1;
    }
    close(compiledFd);
    


    stat(srcFileName, &sb);
    srcLen  = sb.st_size;
    srcTime = sb.st_mtime;
    
    stat(compiledFileName, &sb);
    compLen  = sb.st_size;
    compTime = sb.st_mtime;


    if (srcTime >= compTime) {
        // Needs a compile
        returnVal = srcTime;
    } else {
        returnVal = 0;
    }
    

    return returnVal;
    
}


int writeModule (char * fileName, void * mem, int length)
{
    int fd;
    int i;
    struct utimbuf ubuf;
    unsigned int srcTime;
    
    char realFileName[128];
    char compiledFileName[128];
    char cwd[128];
    
    // Create the compiled directory if necessary
    strcpy(compiledFileName, "Compiled");
    
    if ( access(compiledFileName, R_OK | X_OK | W_OK) ) {
#ifdef WIN32
	i = mkdir(compiledFileName);
#else
	i = mkdir(compiledFileName, 0700);
#endif
        if (i < 0) {
            printf("mkdir [ %s ] failed with errno %d\n", compiledFileName, errno);
            exit(1);
        }
	if ( access(compiledFileName, R_OK | X_OK | W_OK) ) {
	    // Indicate an error
	    getcwd(cwd, sizeof(cwd));
	    
	    printf("[%s]Can't created Compiled directory [%s]\n", cwd, compiledFileName);
	    exit(1);
	}
    }
    
    // Setup the file name
    strcpy(realFileName, "Compiled/");
    strcat(realFileName, fileName);
    strcat(realFileName, ".celmod.tmp");
    
    
    fd = open(realFileName,O_CREAT|O_WRONLY|O_TRUNC, 0777);
    if (-1 == fd) {
      // Indicate an error
      printf("Can't created compiled file [%s]\n", realFileName);
      exit(1);
    }

    i = write(fd, mem, length);

    close(fd);

    if (length != i) {
      // Indicate an error
      printf("Compiled file failed to be written to disk.");
      exit(1);
    }
    
    strcpy(compiledFileName, "Compiled/");
    strcat(compiledFileName, fileName);
    strcat(compiledFileName, ".celmod");
#ifdef WIN32
    i = unlink(compiledFileName);
#endif
    i = rename(realFileName, compiledFileName);
    if (i != 0) {
       printf("Error doing rename [errno = %d]\n", errno);
    }
    
    // Verify that we are ok time wise... 
    // Since we would only be calling write to 
    // write out a compiled version. We can use
    // the checkCompiled function to see if it
    // will work. If it won't, we'll set the time
    // on the compiled file to be 1 second more
    // 
    srcTime = checkCompiled(fileName);
    
    if (0 != srcTime) {
        ubuf.actime  = srcTime + 1;
        ubuf.modtime = srcTime + 1;
        
        utime(realFileName, &ubuf);
    }
    
    return 0;
}


           
MemStream loadModuleStream(char * fileName)
{
    int fd;
    struct stat sb;

    char * p;
    unsigned int len;
    char realFileName[80];
    MemStream ms;
    
    

    strcpy(realFileName, fileName);
    strcat(realFileName, ".cel");
    
    fd = open(realFileName,O_RDONLY, 0);
    if (fd < 0) {
        printf("Couldn't open [%s], source file\n\n", realFileName);
        exit(1);
    }
//    mprintf("Fd: %d\n", (char *)fd);
    
    stat(realFileName, &sb);
    len = sb.st_size;
    
//    mprintf("Size: %d\n", (char *)len);

    p = celmalloc(len); 
    
    read(fd, p, len);
    close(fd);

    ms = NewMemStream(p, len);

    if ((p[0] == '#') && (p[1] == '!')) {
	while (readMSCChar(ms) != '\n') ;
    }

    return (ms);
}


