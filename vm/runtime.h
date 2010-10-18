
//
// runtime.h
//
// This is the description of the functions for the Cel runtime.
// 
//
//
//


#include "svm32.h"
#include "cel.h"
#include "atom.h"
#include "memstream.h"

#ifndef H_CELRUNTIME
#define H_CELRUNTIME


extern char * SearchPaths[];



#define nil   (void *)0
#define True  1
#define False 0




#define PROTO_ACTION_GET          0
#define PROTO_ACTION_SET          1
#define PROTO_ACTION_GETFRAME     2
#define PROTO_ACTION_SETFRAME     3
#define PROTO_ACTION_PASSTHRU     4
// CODE is not used
//#define PROTO_ACTION_CODE         5
#define PROTO_ACTION_INTRINSIC    6




// G L O B A L S   &   H O O K S

// Fill factor for the proto hashes
extern int PROTO_FillFactor;

// Fill factor for the proto hashes
extern Proto Lobby;
extern Proto DefaultBehavior;
extern Proto Globals;
extern Proto Traits;
extern Proto Clonable;
extern Proto ObjectClass;
extern Proto StringParent;
extern Proto ArrayParent;
extern Proto FloatParent;
extern Proto OddBalls;

extern Proto IntegerClass;
extern Proto AtomClass;
extern Proto FloatClass;
extern Proto Capsule;

// Special Objects
extern Proto TrueObj;
extern Proto FalseObj;


extern int debugParse;
extern int debugParseLevel;
extern int debugDump;
extern int debugTrace;
extern int noPrint;
extern int debugTime;


// The default debugging info handler
extern int (*printString)(char * str);        /* Output handler */
extern int (*ioOut)(char * str, int count);   /* IO Output Handler */
extern int (*addPrintSpaces)(int32 spaces);   /* Output Handler */

int runtimeActivateTrace;


extern int runtimeActivateTrace;

extern VM32Cpu * Cpu;


// Preset Atoms - we initialize these since they are used
// so often. PRESET
extern Atom ATOMparent;
extern Atom ATOMblockParent;
extern Atom ATOM_code_;
extern Atom ATOMString;
extern Atom ATOM_string;
extern Atom ATOMsize;
extern Atom ATOMcapacity;
extern Atom ATOMname;
extern Atom ATOM_capacity;
extern Atom ATOM_array;
extern Atom ATOM_float;
extern Atom ATOM_assemble;
extern Atom ATOMop;
extern Atom ATOMnop;
extern Atom ATOMbreak;
extern Atom ATOMslotName;
extern Atom ATOMpop;
extern Atom ATOMswap;
extern Atom ATOMcall;
extern Atom ATOMsetupFrame;
extern Atom ATOMrestoreFrame;
extern Atom ATOMpushSelf;
extern Atom ATOMreturn;
extern Atom ATOMpushimmediate;
extern Atom ATOMargCountCheck;
extern Atom ATOMbranch;
extern Atom ATOMbranchIfTrue;
extern Atom ATOMbranchIfFalse;
extern Atom ATOMoperand;
extern Atom ATOM_Name_;
extern Atom ATOMtype;
extern Atom ATOMobject;
extern Atom ATOMself;
extern Atom ATOMactivateSlot;
extern Atom ATOMdate;
extern Atom ATOM_datetime;




int     celInit(VM32Cpu * cpu);

// This routine re-sets the C language globals with their proper object
// reference values. (These could change after a garbage collection cycle)
int reassignRoots (Proto lobby);


void *  objectNew(int slotNum);
void *  objectNewWithSize(int dataNum, int slotNum);
Proto   objectNewInteger(int i);
// Creates a new string that is a copy of the specified arguments
Proto objectNewFloat (double f);
// Sets an existing float object with a new value
// **kind of a tricky behind the scenes optimization
Proto objectSetFloat (Proto newFloat, double f);
void *  objectNewString(char * str);
void *  objectNewBlob(int size);
// An object blob is what stores object references for hashes, or arrays
void *  objectNewObjectBlob(int num);
// What the size of a blob is
uint32 objectBlobSize (Proto obj);
void *  objectBlobResize (Proto obj, int num);
Proto   objectBlobAtGet(Proto obj, uint32 index);
void    objectBlobAtSet(Proto obj, uint32 index, Proto newItem);

void    objectFree(void * obj);


inline Proto   objectSetAsCodeProto(Proto proto);
inline Proto   objectSetAsBlockProto(Proto proto);
inline int     isObjectSetAsCodeProto(Proto proto);

inline void  * objectResolveForwarders(void * obj);


// Accessing objects -> these follow forwarders before accessing
// object
int32 objectIsType(Proto proto);
//void *  objectForKeyString(void * proto, char * str);
//void *  objectForKey(void * proto, char * key);
//void *  objectSlotAt(void * proto, int slotNum);
void *  objectSetSlot(Proto proto, Atom key, char action, uint32 data);

void *  objectSetDataAtIndex(Proto proto, uint32 index, uint32 data);

// This routine returns the index of the slot for the data
// it throws an assertion if it can't find the slot
// *This is for experienced people only*
// 
int objectGetDataOffsetForSlot(Proto proto, Atom key);

void *  objectEasySetSlot(Proto proto, Atom key, uint32 data);

void *  objectEasySetFrameSlot(Proto proto, Atom key, uint32 data);

void *  objectInsertSlot(Proto proto, uint32 key, uint32 action, uint32 index);

// Set another key/action to point to the same data as an existingKey
void *  objectSetSlotSame(Proto proto, Atom key, Atom existingKey, char action);

// Hash type lookup
inline int objectLookupKey(Proto proto, uint32 key, struct _protoEntry * * slotPtr);

// Accessing data from objects
//int     objectInteger(void * proto);
//void *  objectData(void * proto);

void * objectExpand(Proto proto);

// Enumerating through slots
ProtoSlot objectEnumerate(Proto proto, int * currentSlot, int skipParent);

// This gets the data from a slot, it returns true if it
// finds the key
//// Whether this function will exist is a good question
uint32 objectGetSlot(Proto proto, Atom key, Proto * result);

// This removes a slot from a proto 
// it returns true if it finds the key
uint32 objectDeleteSlot(Proto proto, Atom key);


// This returns a pointer to the data - this only works on blobs
void *  objectPointerValue(Proto proto);

int    objectIntegerValue(Proto proto);

// Return the value of a float object
double objectFloatValue (Proto proto);


inline int objectCapacity(Proto proto);
inline int objectSlotCount   (Proto proto);
inline int objectDataCount   (Proto proto);

// Dump the object to stdio
void *  objectDump(Proto proto);
void *  objectShallowDump(Proto proto);


// The magic function
// This function expects the proto, selector, and arguments on the vm stack
void activateSlot(void);


// locateTargetWithSlotVia
//    This is used by some intriniscs to lookup a slot via inheritance
//    since and intrinsic will sometimes have as a target the context
//    for example (at: 5) print. Will use the 'self' which will be
//    block or code context. I don't want to force people to do '(self at: 5) print.'
//    just yet. This is a sticky issue.
//

Proto locateObjectWithSlotVia (Proto key, Proto obj);


/////////////////////////////////////////////////////
//
// A C T I O N   H A N D L E R S
//
//


// Action Handlers - inlined because only activateSlot should call them
inline void actionGet(Proto object, ProtoSlot slotPtr);

// actionSet assumes that this slot was a 'SET' action. It assumes that the stack
// contained some destination object and the message activated this slot and the
// new object
//
inline void actionSet(Proto object, ProtoSlot slotPtr);


// actionGetStack assumes that this slot was a 'GET' action. It assumes that the stack
// contained some destination object and the message activated this slot
//
inline void actionGetStack(Proto object, ProtoSlot slotPtr, uint32 fp);


// actionSetStack locates the argument in the frame that has to be set and
// sets it to the argument passed in via the immediate stack. For example,
// imagine a local variable (which is stack mapped) in a code proto needs
// to get set. The trick is that the data for the set is in the immediate
// stack frame and the variable is relative to the fp
// now for the two lines of code...:)
//
inline void actionSetStack(Proto object, ProtoSlot slotPtr, uint32 fp);


// actionPassthru deals with activating some code for a passthru slot
//
// This will push the parent if 'useParamAsParent' is true otherwise
// it will retrieve the parent off of the stack. This is the target
// of the message that had the passThru slot.
inline void actionPassthru(Proto object, ProtoSlot slotPtr, Proto parent);


// actionCode - not implemented yet.
//
inline void actionCode(Proto object, ProtoSlot slotPtr);

// actionIntrinsic calls the code via C for the argument in the slot
//
inline void actionIntrinsic(Proto object, ProtoSlot slotPtr);


/////////////////////////////////////////////////////
//
// I N T R I N S I C S
//
//


// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple boolean equality function
void intrinsicTrueEqual(void);


// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple boolean inequality function
void intrinsicTrueNotEqual(void);


// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple boolean equality function
void intrinsicFalseEqual(void);


// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple boolean inequality function
void intrinsicFalseNotEqual(void);



void intrinsicSetSlotWith(void);

// Arguments: target 'slotNames'
// Return: Array of the slotNames
// Notes:
//
void intrinsicSlotNames (void);

// Arguments: target 'hasSlot:' slotName
// Return: boolean
// Notes:
//
void intrinsicHasSlot (void);

// Arguments: target 'getSlot:' slotName
// Return: slot contents
// Notes:
//
void intrinsicGetSlot (void);

// Arguments: target 'removeSlot:' slotName
// Return: target
// Notes:
// Remove the slot

void intrinsicRemoveSlot (void);

// Arguments: target 'copySlotsFrom:' proto
// Return: target
// Notes:
// Copies the slots from the proto argument

void intrinsicCopySlotsFrom (void);


// Arguments: target 'perform:' slotName
// Return: slot or method's contents
// Notes:
//
void intrinsicPerform (void);


// Arguments: target 'traceOn/traceOff'
// Return: target
// Notes:
// This turns message tracing On or Off
void intrinsicTrace (void);

// Arguments: target 'isNil'
// Return: true/false
void intrinsicObjectIsNil(void);

// Arguments: target 'clone'
// Return: new clone
void intrinsicClone(void);

// Arguments: target <someMessage>
// Return: new clone
// Notes:
// Assumes some binary message, just returns the target, the self
void intrinsicSelf(void);

// Arguments: target '+' argument
// Return: newValue
// Notes:
// Simple add function - imagine the hardware to iplement this versus
// a simple adder circuit
void intrinsicIntegerAdd(void);


// Arguments: target '-' argument
// Return: newValue
// Notes:
// Simple integer subtract function
void intrinsicIntegerSubtract(void);

// Arguments: target '*' argument
// Return: newValue
// Notes:
// Simple integer multiply function
void intrinsicIntegerMultiply(void);

// Arguments: target '/' argument
// Return: newValue
// Notes:
// Simple integer divide function
void intrinsicIntegerDivide(void);

// Arguments: target '%' argument
// Return: newValue
// Notes:
// Simple integer modulo function
void intrinsicIntegerModulo(void);

// Arguments: target 'abs'
// Return: newValue
// Notes:
// Simple integer absolute value function
void intrinsicIntegerAbsolute(void);

// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple integer equality function
void intrinsicIntegerEqual(void);

// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple integer inequality function
void intrinsicIntegerNotEqual (void);

// Arguments: target '>' argument
// Return: true/false
// Notes:
// Simple integer greater-than function
void intrinsicIntegerGreater(void);

// Arguments: target '>=' argument
// Return: true/false
// Notes:
// Simple integer greater-than or equal function
void intrinsicIntegerGreaterEqual(void);

// Arguments: target '<' argument
// Return: true/false
// Notes:
// Simple integer less-than function
void intrinsicIntegerLess(void);

// Arguments: target '<=' argument
// Return: true/false
// Notes:
// Simple integer less-than or equal function
void intrinsicIntegerLessEqual(void);

// Arguments: target '@' argument
// Return: newValue
// Notes:
// Simple point create function 
void intrinsicPointCreate(void);

// Arguments: target 'print'
// Return: target
// Notes:
// Prints to the default string print routine
void intrinsicIntegerPrint(void);

// Arguments: target 'asFloat'
// Return: new float
// Notes:
// This routine tries to return the argument as a float
// Otherwise it returns a nil
void intrinsicIntegerAsFloat (void);

// Arguments: target 'asString'
// Return: target
// Notes:
// This also handles 'asStringWithFormat' 
void intrinsicIntegerAsString(void);



// Arguments: target
// Return: newValue
// Notes:
// Simple proto copy
void intrinsicFloatCopy(void);

// Arguments: target '+' argument
// Return: newValue
// Notes:
// Simple add function - imagine the hardware to implement this
// routine via the VM versus a simple adder circuit
void intrinsicFloatAdd(void);

// Arguments: target '-' argument
// Return: newValue
// Notes:
// Simple integer subtract function
void intrinsicFloatSubtract(void);

// Arguments: target '*' argument
// Return: newValue
// Notes:
// Simple integer multiply function
void intrinsicFloatMultiply(void);

// Arguments: target '/' argument
// Return: newValue
// Notes:
// Simple integer divide function
void intrinsicFloatDivide(void);

// Arguments: target 'abs'
// Return: newValue
// Notes:
// Simple integer absolute value function
void intrinsicFloatAbsolute(void);

// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple integer equality function
void intrinsicFloatEqual (void);

// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple integer inequality function
void intrinsicFloatNotEqual (void);

// Arguments: target '>' argument
// Return: true/false
// Notes:
// Simple integer greater-than function
void intrinsicFloatGreater(void);

// Arguments: target '>=' argument
// Return: true/false
// Notes:
// Simple integer greater-than or equal function
void intrinsicFloatGreaterEqual(void);

// Arguments: target '<' argument
// Return: true/false
// Notes:
// Simple integer less-than function
void intrinsicFloatLess(void);

// Arguments: target '<=' argument
// Return: true/false
// Notes:
// Simple integer less-than or equal function
void intrinsicFloatLessEqual(void);

// Arguments: target 'print'
// Return: target
// Notes:
// Prints to the default string print routine
void intrinsicFloatPrint(void);

// Arguments: target 'asString'
// Return: target
// Notes:
// This also handles 'asStringWithFormat' 
void intrinsicFloatAsString(void);

// Arguments: target 'asInteger'
// Return: target
// Notes:
void intrinsicFloatAsInteger(void);

// Arguments: target 'asIntegerScaled:'
// Return: target as an integer after being multiplied by an integer scalar
// Notes:
void intrinsicFloatAsIntegerScaled(void);



// Arguments: target 'import:' argument
// Return: target
// Notes:
// Loads in that Kit
void intrinsicKitImport(void);

// A little helper function for kit linking
void linkLib (Proto kit, char * name);


// A support method for KitImport
Proto intrinsicLoadModule(Atom fileName);

// Arguments: target 'importLib:' argument
// Return: target
// Notes:
// Loads in that Kit's library
void intrinsicKitImportLib (void);

// Arguments: target 'value'
// Return: NONE - the block does the return
// Notes:
// This changes the thread of control to a block
void intrinsicValue(void);

// Arguments: target 'whileTrue:' obj
// Return: NONE - the block does the return
// Notes:
// This changes the thread of control to a WhileTrue block
void intrinsicWhileTrue (void);

// Arguments: target 'whileFalse:' obj
// Return: NONE - the block does the return
// Notes:
// This changes the thread of control to a WhileFalse block
void intrinsicWhileFalse (void);


// Arguments: target 'print'
// Return: target
// Notes:
// Prints to the default string print routine
void intrinsicAtomPrint(void);


// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple atom equality function
void intrinsicAtomEqual(void);

// Arguments: target '!=' argument
// Return: true/false
// Notes:
// Simple atom inequality function
void intrinsicAtomNotEqual(void);

// Arguments: target '>' argument
// Return: true/false
// Notes:
// Simple atom greater-than function
void intrinsicAtomGreater(void);

// Arguments: target '>=' argument
// Return: true/false
// Notes:
// Simple atom greater-than or equal function
void intrinsicAtomGreaterEqual(void);

// Arguments: target '<' argument
// Return: true/false
// Notes:
// Simple atom less-than function
void intrinsicAtomLess(void);

// Arguments: target '<=' argument
// Return: true/false
// Notes:
// Simple atom less-than or equal function
void intrinsicAtomLessEqual(void);

// Arguments: target 'size'
// Return: integer size
// Notes:
// Returns the size of the string
void intrinsicAtomSize(void);

// Arguments: target 'asString'
// Return: a new string
// Notes:
// Create a new String object out of this literal
void intrinsicAtomAsString(void);


// Arguments: target 'objectDump'
// Return: junk
void intrinsicObjectDump(void);

// Arguments: target 'objectShallowDump'
// Return: junk
void intrinsicObjectShallowDump(void);


// Arguments: target 'print'
// Return: self/target
// Notes:
// Prints to the default string print routine
void intrinsicStringPrint(void);

// Arguments: target '+' literal/string
// Return: new string that is the combination
// Notes:
// Creates a new string that is the concatenation of the target and the argument
void intrinsicStringAdd(void);

// Arguments: target 'clear' position
// Return: the cleared string
// Notes:
void intrinsicStringClear(void);

// Arguments: target 'at:' position
// Return: the character literal at that position
// Notes:
void intrinsicStringAt(void);

// Arguments: target 'insert:' literal/string 'at:' position
// Return: string that is the combination
// Notes:
// Returns the string with that insertion at that position
void intrinsicStringInsertAt(void);

// Arguments: target 'removeAt:' position
// Return: string that has the specified position removed
void intrinsicStringRemoveAt(void);

// Arguments: target 'removeChar:' stringliteral
// Return: self without the characters
// Notes:
// Only first character of string literal is used
void intrinsicStringRemoveChar(void);

// Arguments: target 'append:' literal/string
// Return: self
// Notes:
// Just appends a string on to the existing target
void intrinsicStringAppend(void);

// Arguments: target 'reverse'
// Return: self
// Notes:
// Reverses the string in place
void intrinsicStringReverse(void);

// Arguments: target 'lowercase'
// Return: self
// Notes:
// Makes each character lower case in the string
void intrinsicStringLowercase(void);

// Arguments: target 'asAtom'
// Return: variable as an atom
// Notes:
// This routine may create an atom if necessary
void intrinsicStringAsAtom(void);

// Arguments: target 'setCapacity:' integer
// Return: new string that is of that capacity
// Notes:
// Creates a new string that is has that capacity
void intrinsicStringSetCapacity(void);


// Cel strstr - our own version that works on blobs rather than null-terminated
// sequences
char *celStrStr(char *haystack, int haySize,char *needle, int needleSize);

// Same as above but searches from last part of string
char *celRevStrStr(char *haystack, int haySize, char *needle, int needleSize);


// Arguments: target 'setCapacity:' integer
// Return: new string that is of that capacity
// Notes:
// This routine handles splitOn: and splitOn:atMostCount:
// count: 0 = stop at that number of splits is has that capacity
void intrinsicStringSplit(void);

// Arguments: target 'splitCSV'
// Return: new array of strings delimited in the Comma Separated Values format
// Notes:
void intrinsicStringSplitCSV(void);

// Copies the string from an Atom or String into the specified buffer
int getString(Proto target, char * buff, int bsize);

// Copies the string from an Atom or String into the specified buffer
int getStringPtr(Proto target, char ** buff, int * bsize);

// Creates a C-String from the specified Cel String/Atom
// Note, this requires the particular customer to use celfree
// on the string
char * string2CString (Proto target);

// Get an atom from an atom or a string
Atom getAtom(Proto target);

// Creates a new string that is a copy of the specified arguments
Proto createString(char * buff, int size);

// Sets an existing string with new data
// Avoids creating an object, it copies write into
// the existing string's buffer memory
void setString (Proto str, char * buff, int size);


// Verifies that proto is string or atom
int isStringOrAtom (Proto target);


// Arguments: target 'copyFrom:' integer 'To:' integer
// Return: new string that is the slice of the original
// Notes:
// Currently it doesn't deal with negative offsets, but
// it will
void intrinsicStringSlice(void);

// Arguments: target 'asInteger'
// Return: new integer
// Notes:
// This routine tries to return the argument as an integer
// Otherwise it returns a nil
void intrinsicStringAsInteger(void);

// Arguments: target 'asIntegerScaled:' integer
// Return: new integer
// Notes:
// This routine converts a float to an integer after
// multiplying it by a scalar
void intrinsicStringAsIntegerScaled(void);

// Arguments: target 'asFloat'
// Return: new integer
// Notes:
// This routine tries to return the argument as a float
// Otherwise it returns a nil
void intrinsicStringAsFloat(void);

// Arguments: target 'findLast:' argument
// Return: new integer
// Notes:
// This routine tries to find the argument from right to left
// Otherwise it returns a nil
void intrinsicStringFindLast(void);

// Arguments: target 'urlUnEscape'
// Return: self
// Notes:
// This routine unescapes the characters in a URL
// '+' is changed to a space and % hex hex is changed to that character
void intrinsicStringUrlUnEscape(void);

// Arguments: target 'urlEscape'
// Return: self
// Notes:
// This routine escapes the characters in a URL
// '+' is changed to a space and % hex hex is changed to that character
//// DRUDRU - for now space is just changed to '+'
void intrinsicStringUrlEscape(void);

// Arguments: target '=' argument
// Return: true/false
// Notes:
// Simple string equality function
void intrinsicStringEqual(void);

// Arguments: target '>' argument
// Return: true/false
// Notes:
// Simple string greater-than function
void intrinsicStringGreater(void);

// Arguments: target '>=' argument
// Return: true/false
// Notes:
// Simple string greater-than or equal function
void intrinsicStringGreaterEqual(void);

// Arguments: target '<' argument
// Return: true/false
// Notes:
// Simple string less-than function
void intrinsicStringLess(void);

// Arguments: target '<=' argument
// Return: true/false
// Notes:
// Simple string less-than or equal function
void intrinsicStringLessEqual(void);


// Arguments: target 'new'
// Return: a new array
// Notes:
// Return a new array instance
void intrinsicArrayNew (void);

// Arguments: target 'new:' <integer size>
// Return: a new array of size
// Notes:
// Return the array position at: x
void intrinsicArrayNewSize (void);

// Arguments: target 'at:' integer
// Return: value in array
// Notes:
// Return the array position at: x
void intrinsicArrayAt(void);

// Arguments: target 'insert:' obj 'at:' integer 
// Return: self
// Notes:
// Insert an object at position
void intrinsicArrayInsertAt(void);

// Arguments: target 'at:otherwiseUse:' integer
// Return: value in array
// Notes:
// Return the array position at: x
void intrinsicArrayAtOtherwiseUse(void);

// Arguments: target 'at:' integer 'Put:' obj
// Return: self
// Notes:
// Place an object at position
void intrinsicArrayAtPut(void);

// Arguments: target 'removeAt:' integer
// Return: self
// Notes:
// Remove an object at position and slide the rest to the left
void intrinsicArrayRemoveAt(void);

// Arguments: target 'append:' object
// Return: self
// Notes:
// Append an object to the end of the array
void intrinsicArrayAppend(void);

// Arguments: target 'getFromFile:' argument
// Return: new structure or nil
// Notes:
// Loads in that simpledata
void intrinsicSDGetFromFile (void);

// Arguments: target 'saveToFile:' argument
// Return: target
// Notes:
// 
void intrinsicSDSaveToFile (void);

// Arguments: target 'fromString:' argument
// Return: new structure or nil
// Notes:
// Loads in that simpledata from a string
void intrinsicSDfromString (void);

// Arguments: target 'asString:'
// Return: target
// Notes:
// Converts protos to a SimpleData blob in a string
void intrinsicSDasString (void);

// Arguments: target 'run'
// Return: nil
// Notes:
// Forces a collection
void intrinsicGCrun (void);

// Arguments: target 'isOn'
// Return: nil
// Notes:
// Returns status of gcIsOn variable
void intrinsicGCisOn (void);



// U S E R  S U P P O R T functions


void doCelInit (char * module, VM32Cpu * pcpu, int32 noRun);

Proto loadModule(char * fileName);


int  checkCompiled(char * fileName);

MemStream loadModuleStream(char * fileName);

int  writeModule(char * fileName, void * mem, int length);


#endif
