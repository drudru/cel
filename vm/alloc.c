
#include "alloc.h"
#include "cel.h"
#include "runtime.h"
#include "stack.h"
#include "stackframe.h"
#include "bitstring.h"



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//  B I N S

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// This is the shared-interface for bins
struct CommonBin
{
    uchar        binType;	// anything > 127 is an overbin(in 4k pages)
    uchar        notUsed[3];    // We have to stay on 8 byte alignment
    struct CommonBin * nextBin;	// Next bin on list
};


typedef  struct CommonBin * CommonBin;



#define MIN_OBJ_SIZE  16
// Would be nice to at least handle 4k blocks // XXX
//#define MAX_OBJ_SIZE  4096
#define MAX_OBJ_SIZE  4080
#define MIN_BIN_SIZE  1
#define MAX_BIN_SIZE  4
#define MAX_BITS      ((MIN_BIN_SIZE * 4096) / MIN_OBJ_SIZE)
#define MAX_BITS_AS_BYTES     ( MAX_BITS / 8 )


// This is a bin
// For objects under 4k, 

struct Bin
{
    uchar        binSize;	// (in 4k pages)
    uchar        objSize;	// (in multiples of 16)
    uchar        notUsed[2];    // We have to stay on 8 byte alignment
    struct Bin * nextBin;	// Next bin on list
    uchar        allocBits[MAX_BITS_AS_BYTES];
    uint32       data;
};

typedef  struct Bin * Bin;

#define BIN_MAX(b)  ( ((b)->binSize << 12) / ((b)->objSize << 4) )


Bin  Bins[256];



// These bins are for objects over the maximum object size
// They are in 8k chunks
#define MAX_OVERBIN_SIZE          (8192)
#define MAX_OVERBITS              (MAX_OVERBIN_SIZE / 4)
#define MAX_OVERBITS_AS_BYTES     ( MAX_OVERBITS / 8 )

struct OverBin
{
    uchar        binSize;	// *** This is used as a type ***
    uchar        notUsed[3];    // We have to stay on 8 byte alignment
    struct OverBin * nextBin;	// Next bin on list
    uchar        allocBits[MAX_OVERBITS_AS_BYTES];
    uint32       data;
    
};

typedef  struct OverBin * OverBin;

#define OVERBIN_MAX (b)  ( MAX_OVERBITS )



OverBin  OverBins;
OverBin  LastFreeOverBin;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////





uint32 gcBaseSet;
uint32 gcResidentSet;
Stack  gcMarkStack;
uint32 gcRuns;


static char gcIsOn;





/* Globals */




/* 
  Debugging support 
*/

int (*mallocDebugHook) (int type,int mem) = NULL;  /* Debug Message Handler */




// Main Routines

void * celmalloc(uint32 bytes)
{
    void * p;
    
    p = malloc(bytes);
    assert(p);
    // Debug hook
    if (mallocDebugHook) {
	mallocDebugHook(0, bytes);
    }

    return p;
}

void celfree (void * memory)
{
    free(memory);
}

void * celcalloc (uint32 bytes)
{
    void * p;
    
    p = calloc(1, bytes);
    assert(p);
    return p;
}


void *  celrealloc (void * mem, uint32 bytes)
{
    return realloc(mem, bytes);
}


/*
 * GC routines for right now.  Do nothing, but increase the levels
 */
void gcInit(void)
{
    int i;
    
    gcBaseSet     = 0;
    gcResidentSet = 0;
    gcRuns        = 0;
    gcMarkStack   = StackNew(8192); // Depth of 2048

    for (i = 0; i < 256; i++) {
	Bins[i] = nil;
    }
    
    OverBins = nil;
    
    gcIsOn = 1;
}


void gcOFF (void)
{
    gcIsOn--;
}



void gcON (void)
{
    gcIsOn++;
    assert(gcIsOn <= 1);
}

char gcGetIsOn (void)
{
    return gcIsOn;
}


Proto gcMarkPush (Proto p)
{

    if (PROTO_ISFORWARDER(p)) {
	p = objectResolveForwarders(p);
    }

    // NOTE: the order is really important here, we don't
    // want to mark the forwarder! So, we mark after the
    // the forwarder is resolved
    PROTO_SET_MARK(p);

    gcBaseSet += PROTO_GET_SIZE(p);
    
    // if object has no children
    if (PROTO_HAS_REFERENCES(p)) {
	StackPush(gcMarkStack, (uint32) p);
    }

    return p;
}


// **It is assumed that all items on the MarkStack
// **have children and can be followed
// **It is also assumed that they aren't pointing to 
// **forwarders.
// **It is also assumed that they were marked before the push
void gcMark ()
{
    Proto p;
    Proto c;
    int   i;
    char  j;
    int   size;
    ProtoSlot  slotPtr;
    
    
    
    while (!StackIsEmpty(gcMarkStack)) {

	p = (Proto) StackPull(gcMarkStack);

	// if proto is a proto, ie. not a blob
	if (PROTO_REFTYPE_PROTO(p)) {
	    
	    i = 0;
	    while (1) {
		slotPtr = objectEnumerate(p, &i, 0);

		if (slotPtr == nil) {
		    break;
		}
		    
		j = slotAction(slotPtr);
		
		// Only certain actions are important
		if ((j == PROTO_ACTION_GET) || (j == PROTO_ACTION_PASSTHRU)) {
		    
		    c = protoSlotData(p, slotPtr);

		    if (PROTO_HAS_REFERENCES(c) && (PROTO_GET_MARK(c) == 0)) {
			protoSlotData(p, slotPtr) = gcMarkPush(c);
		    }
		}
	    }
	    // If it's a blob, check if it is clear
	} else if (PROTO_ISCLEAR(p)) {
	    size = objectBlobSize(p);
	    
	    // Size is returned in bytes, so scale it down since
	    // the index represents a 4 byte quantity
	    size = size >> 2;
	    
	    // enumerate through the blob
	    for (i = 0; i < size; i++) {
		c = objectBlobAtGet(p, i);
		
		if (PROTO_HAS_REFERENCES(c) && (PROTO_GET_MARK(c) == 0)) {
		    c = gcMarkPush(c);
		    objectBlobAtSet(p, (uint32) i, c);
		}
	    }
	}
    }
}



//
// Create a new bin
//
Bin gcCreateBin (int objSize, int binSize)
{
    Bin    bin;
    
    bin = (Bin) celcalloc((sizeof(struct Bin) + 4096 * binSize));
    bin->binSize = binSize;
    bin->objSize = objSize;
    bin->nextBin = nil;

    return bin;
}


//
// Create a new bin
//
OverBin gcCreateOverBin ()
{
    OverBin    bin;
    
    bin = (OverBin) celcalloc((sizeof(struct OverBin) + MAX_OVERBIN_SIZE));
    bin->binSize = 128;
    bin->nextBin = nil;

    return bin;
}


//
// Store in a free bin, create a bin if necessary
//
Proto gcStoreInBin (uint32 size)
{
    int  * ip;
    Bin    bin;
    uint32 i;
    int    j;
    int    index;
    
    // Use 15 in order to round up 
    i = (size + 15) / 16;
    i--;
    
    
    if (Bins[i] == nil) {
	// Create a bin
	j = size / 256;
	if (j == 0) {
	    j = 1;
	}
	
	bin = Bins[i] = gcCreateBin((i+1), j);
	index = 0;
    } else {
	uint32  max;
	    
	bin = Bins[i];
	
	max = BIN_MAX(bin);
	
	// Locate a bin
	while (1) {
	    // locate the next clear bit
	    bit_ffc(bin->allocBits, max, &index);
	    if (index >= 0) {
		break;
	    }
	    if (bin->nextBin == nil) {
		// Use that bin for the last time
		bin = gcCreateBin((i+1), bin->binSize);
		// Make the bin the new head of the list
		bin->nextBin = Bins[i];
		Bins[i] = bin;
		index = 0;
		break;
	    }
	    bin = bin->nextBin;
	}
    }


    // Ok, now that we have a bin, prep the object inside of it
    ip = (int *)(&bin->data);
    // Multiply object index by (size * 4)
    // (since size is in multiples of 16 and the pointer moves in 
    //  increments of 4)
    ip += index * (bin->objSize << 2);

    // Mark object as used
    bit_set(bin->allocBits, index);

    // Store the size in the object (which is on a scale of 16)
    *ip = bin->objSize << 4;

    // Increase the resident set size
    gcResidentSet += *ip;

    return (Proto) ip;
}


//
// Store in a free bin, create a bin if necessary
//
Proto gcStoreInOverBin (uint32 size)
{
    int      * ip;
    Proto      p;
    OverBin    bin;
    int        index;
    
    if (OverBins == nil) {
	
	bin = OverBins = gcCreateOverBin();

	index = 0;
    } else {
	bin = OverBins;
	
	// Locate a bin
	while (1) {
	    // locate the next clear bit
	    bit_ffc(bin->allocBits, MAX_OVERBITS, &index);
	    if (index >= 0) {
		break;
	    }
	    if (bin->nextBin == nil) {
		// Use that bin for the last time
		bin = gcCreateOverBin();
		// Make the bin the new head of the list
		bin->nextBin = OverBins;
		OverBins = bin;
		index = 0;
		break;
	    }
	    bin = bin->nextBin;
	}
    }

    // NORMALIZE the size to be on a 16 byte alignment
    size = size + 15;
    size = size & PROTO_SIZEFIELD;

    // Allocate the object and store the size within it
    p = (Proto) celcalloc(size);
    p->header = size;
    
    // Ok, now that we have a bin, find the slot
    ip = (int *)(&bin->data);
    // (since size is in multiples of 16 and the pointer moves in 
    //  increments of 4) all we need to do is add the index
    ip += index;

    // Mark object as used
    bit_set(bin->allocBits, index);

    // Store the reference to the object
    *ip = (uint32) p;

    // Increase the resident set size
    gcResidentSet += size;
    
    
    return (Proto) p;
}



void gcFinishBin (CommonBin b)
{
    // Do nothing for now, we don't know how to remove
    // the memory from the system (maybe we need doubly
    // linked lists)

    return;
}


void gcFree (CommonBin b, int i)
{
    void * p;
    int  * ip;

    // Free object
    
    // If it's a small bin
    if ( (b->binType & 0x80) == 0 ) {
	Bin bin = (Bin) b;
	
	if ( bit_test(bin->allocBits, i) ) {
	    // clear the allocation bit
	    bit_clear(bin->allocBits, i);

	    // Also clear the memory 
	    // We do this immediately to understand that the memory
	    // is freed

	    // Ok, now that we have a bin, prep the object inside of it
	    ip = (int *)(&bin->data);
	    // Multiply object index by (size * 4)
	    // (since size is in multiples of 16 and the pointer moves in 
	    //  increments of 4)
	    ip += i * (bin->objSize << 2);
	    
	    memset((void *) ip, 0x0, (bin->objSize << 4));
	}
	else {
	    // Can't free already free object
	    assert(0);
	}
    }
    else {
	// Although I could name it 'bin', I bet that
	// debuggers would give me trouble later on
	OverBin obin = (OverBin) b;
	
	// if current object is allocated
	if ( bit_test(obin->allocBits, i) ) {
	    // clear the allocation bit
	    bit_clear(obin->allocBits, i);
	    
	    p = &obin->data;
	    // Over bin just contains 4 byte pointers
	    (char *)p += i << 2;
	    celfree( p );
	}
	else {
	    // Can't free already free object
	    assert(0);
	}
    }
}


// This routine returns the next object that is allocated in
// the bin. This routine is used by the Sweep function
// The i must start at -1 since the function returns with
// i containing the index of the found allocated object
int gcBinNextObj (CommonBin b, int32 * i, Proto * protoPtr)
{
    void * p;

    // Initialize the counter
    *i = *i + 1;

    
    // If it's a small bin
    if ( (b->binType & 0x80) == 0) {
	Bin bin = (Bin) b;
	uint32  max;
	
	max = BIN_MAX(bin);
	
	while (1) {
	    if ( *i > max ) {
		return 0;
	    }
	    
	    // if current object is allocated
	    if ( bit_test(bin->allocBits, *i) ) {
		p = &bin->data;
		// Multiply object index by (size * 16)
		// (since size is in multiples of 16)
		(char *)p += *i * (bin->objSize << 4);
		*protoPtr = (Proto) p;

		return 1;
	    } 
	    else {
		*i = *i + 1;
		// locate the next set bit
		//// DRUDRU - the following is a bug
		//// I thought it would start at i, I was wrong
		//bit_ffs(bin->allocBits, max, i);
	    }
	}
    }
    else {
	// Although I could name it 'bin', I bet that
	// debuggers would give me trouble later on
	OverBin obin = (OverBin) b;

	while (1) {
	    if ( *i > MAX_OVERBITS ) {
		return 0;
	    }
	    
	    // if current object is allocated
	    if ( bit_test(obin->allocBits, *i) ) {

		p = &obin->data;
		// Over bin just contains 4 byte pointers
		(char *)p += *i << 2;
		*protoPtr = * ( (Proto *) p);
		
		return 1;
	    } 
	    else {
		*i = *i + 1;
		// locate the next set bit
		//// DRUDRU - the following is a bug
		//// I thought it would start at i, I was wrong
		//bit_ffs(obin->allocBits, MAX_OVERBITS, i);
	    }
	}
    }
}



void gcSweepBin (CommonBin bin)
{
    Proto  obj;
    int32  idx;
    CommonBin currentBin;
    
    currentBin = bin;
    
    while (1) {

	// gcBinNextObj requires that idx start at -1
	idx = -1;
	while ( gcBinNextObj(currentBin, &idx, &obj) ) {

	    // IF unmarked, free it
	    if ( 0 == PROTO_GET_MARK(obj) ) {
		gcFree(currentBin, idx);
	    } 
	    // IF marked, just unmark it for the next pass
	    else {
		PROTO_SET_UNMARK(obj);
	    }
	}

	// Do any handling that needs to be done after
	// a bin has been processed
	gcFinishBin(currentBin);
	
	currentBin = currentBin->nextBin;
	if (currentBin == 0) {
	    break;
	}
    }
}


void gcSweep (void)
{
    int i;
    
    // Sweep bins
    for (i = 0; i < 256; i++) {
	if (Bins[i]) {
	    gcSweepBin( (CommonBin) Bins[i]);
	}
    }

    if (OverBins) {
	gcSweepBin( (CommonBin) OverBins);
    }
}


int gcBinAvail (uint32 size)
{
    uint32 i;
    
    size += 15;
    
    i = size / 16;

    return (Bins[i] != nil);
}


void gcMarkSweep (void) 
{
    Proto  p;
    uint32 i;
    StackFrame stackFrame;
    
    // Roots: local variables on stack, (no CPU registers since we're stack
    //        based, global vars - the Lobby

    // Stack references from Low Memory to High == Top of the Stack to the Bottom

    stackFrame = StackFrameNew(Cpu);
    
    while (1) {

	for (i = 0; i < stackFrame->numArgs; i++) {
	    p = stackFrame->args[i];
	    
	    if (PROTO_HAS_REFERENCES(p)) {
		stackFrame->args[i] = gcMarkPush(p);
	    }
	}

	if ( ! stackFrame->isIntrinsic) {

	    // Note, some of these aren't locals, they may
	    // just be on the stack in preparation for being
	    // parameters in another call
	    for (i = 0; i < stackFrame->numLocals; i++) {
		p = stackFrame->locals[i];
		
		if (PROTO_HAS_REFERENCES(p)) {
		    stackFrame->locals[i] = gcMarkPush(p);
		}
	    }
	
	    p = *(stackFrame->realTarget);
	    if (PROTO_HAS_REFERENCES(p)) {
		*(stackFrame->realTarget) = gcMarkPush(p);
	    }

	    p = *(stackFrame->codeProto);
	    if (PROTO_HAS_REFERENCES(p)) {
		*(stackFrame->codeProto)  = gcMarkPush(p);
	    }

	}

	if (StackFrameNext(stackFrame) == 0) {
	    break;
	}
    }

    StackFrameFree(stackFrame);
 
    // Lobby reference
    Lobby = gcMarkPush(Lobby);
    

    gcBaseSet = 0;
    
    gcMark();
    gcSweep();

    gcResidentSet = gcBaseSet;
    
    // Other globals (like 'Globals' or TrueObj ) need to be reset
    reassignRoots(Lobby);
    

//    if (memory size)
//      allocate more memory
    
}


static char buff[128];

void gcRun(void) 
{
    //sprintf(buff, "ATTEMPT GCRUN %06ld  Set: %09ld\n", gcRuns, (gcResidentSet - gcBaseSet) );
    //printString(buff);

    if (gcIsOn == 1) {
	gcMarkSweep();
	if (debugTrace) {
	    sprintf(buff, "GCRUN %06ld\n", gcRuns);
	    printString(buff);
	}
	gcRuns++;
    }
}


/*
  ObjAlloc Algorithm:
*/

void * celObjAlloc(uint32 sizeBytes)
{
    void * p;
    
    // Some routines allocate zero length blobs (ex: string splits)
    if (sizeBytes == 0) {
	sizeBytes = 1;
    }

    // Add 4 for the size header in the proto-struct
    sizeBytes += 4;
    
    if (sizeBytes <= MAX_OBJ_SIZE) {
	
	// Quick look for a free bin?
//	if ( ! gcBinAvail(sizeBytes)) {
	// If we are over our limit, then do a GC run
	if ( ((gcResidentSet + sizeBytes) - gcBaseSet) > (1024 * 1024) ) {
	    gcRun();
	}
//	}
	
	// This will create a bin if necessary and adjust the gcResidentSet size
	// and return a pointer to the memory
	p = gcStoreInBin(sizeBytes);

	return p;
    }
    else {
	// If we are over our limit, then do a GC run
	if ( ((gcResidentSet + sizeBytes) - gcBaseSet) > (1024 * 1024) ) {
	    gcRun();
	}

	// This will create a bin if necessary and adjust the gcResidentSet size
	p = gcStoreInOverBin(sizeBytes);

	return p;
    }
}



