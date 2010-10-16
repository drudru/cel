
#include "celmalloc.h"


/*
 Malloc works by manipulating 'chunks'. This is the name used by
 older malloc's, so it sticks. At any rate.
 A chunk has some information in it's header that is metadata for the
 chunk (size, etc.).
 
 These routines keep track of what pages are used so the garbage 
 collector will know what to search through. 
*/

/* 8/26/98 -

   See Bubl.h for the description of the bit fields.

   */

/*
  Type declarations
*/


struct malloc_chunk
{
    uint32        size;		/* Size in bytes of malloc (including this field) */
    struct malloc_chunk * fd;   /* double links -- used only if free.  */
    struct malloc_chunk * bk;
};

typedef struct malloc_chunk* mchunkptr;

typedef struct malloc_chunk * Chunk;


// This struct holds a begining page and the number of consecutive
// pages after it that are used after it.
struct pageRange
{
    char    * page;
    uint32    size;
};

struct pagesRecord
{
    struct pageRange next;	// This holds the pointer to the next page and the number of entries
    struct pageRange entry[511];
};


/*  sizes, alignments */

#define SIZE                   (4)
#define MALLOC_ALIGNMENT       (4*SIZE)
#define MALLOC_ALIGN_MASK      (MALLOC_ALIGNMENT - 1)
#define MINSIZE                (16)
/* probably 64k for a 4k page size */
#define MINPAGES               (16)

#define  PageSize 4096
#define  PageMask (PageSize - 1)



/* size field is or'ed with PREV_INUSE when previous adjacent chunk in use */

#define PREV_INUSE 0x1 

/* size field is or'ed with GC_INUSE when this chunk is garbage collected */

#define GC_INUSE 0x2

/* This indicates that the chunk is a forwarder for the real data -
   the ptr is in the 'fd' link. */

#define FORWARDER  (0x00000008)


/* Bits to mask off when extracting size */

#define SIZE_MASK (0x8000000F)
#define SIZE_BITS (~SIZE_MASK)


/* helpful macros */

#define ChunkSize(p)          ((p)->size & SIZE_BITS)

#define Chunk2Mem(p)          ((void *)((char *)(p) + SIZE))

#define Mem2Chunk(p)          ((void *)((char *)(p) - SIZE))






#define MIN_OBJ_SIZE  16
#define MAX_OBJ_SIZE  4096
#define MAX_BIN_SIZE  4
#define MAX_BITS      (MAX_OBJ_SIZE / MIN_OBJ_SIZE)
#define MAX_BITS_AS_BYTES     ( MAX_BITS / 8 )

struct Bin
{
    uchar        binSize;	// (in 4k pages)
    uchar        objSize;	// (in multiples of 
    uchar        allocBits[MAX_BITS_AS_BYTES];
    
    struct Bin * nextBin;	// Next bin on free/full list
    
    uint32     * data;
    
}

typedef  struct Bin  Bin;

#define BIN_MAX (b)  ( (b).binSize / ((b).objSize << 4) )


extern Bin  freeBins[256];
extern Bin  fullBins[256];



// These bins are for objects over the maximum object size
// They are in 8k chunks

#define MAX_OVER_BITS_AS_BYTES     ( MAX_BITS / 8 )

struct OverBin
{
    uchar             allocBits[MAX_BYTES];
    
    struct OverBin  * nextBin;	// Next bin on free/full list

    uint32          * data;
    
};


typedef  struct Bin  Bin;

extern OverBin  * overBins;
extern OverBin  * lastFreeOverBin;


extern uint32 gcBaseSet;
extern uint32 gcResidentSet;





/* Globals */

static uint32               TopBytesLeft = 0;
static char               * Top = nil;

struct pagesRecord * Pages = nil;		// Root pointer to linked list of pages







/* 
  Debugging support 
*/

int (*mallocDebugHook) (int type,int mem) = NULL;  /* Debug Message Handler */




// Support routines


// This records what pages were used
// Each page holds entries consisting of 4 byte page pointer
// with 2 byte size.
// This stores the first page and then the number of pages
// in the setThe first two entries
// are the link to the next page and the count of entries used
// in that page
void recordPages(void * page, int numPages)
{
    struct pagesRecord * p;
    uint32   i;
    
    

    // If this is the first time, get a page
    if (Pages == nil) {
	Pages = (struct pagesRecord *) calloc(1, PageSize);
	//// We should throw a nasty exception here
	assert(Pages);
    }

    // Follow the chain to the last page
    p = Pages;

    while (p->next.page != nil) {
	p = (struct pagesRecord *) p->next.page;
    }

    

    // If there is room, record what pages can be recorded
    i = p->next.size;
    
    if (i == 511) {
	// Otherwise, allocate another page and add it to the list
	p->next.page = (char *)calloc(PageSize, 1);
	assert(p->next.page);
	i = 0;
	p = (struct pagesRecord *) p->next.page;
    }

    p->entry[i].page = page;
    p->entry[i].size = numPages;

    p->next.size++;
    
}



// This extends the top segment by 
uint32 addBytesTop(uint32 bytes)
{
    uint32   pagesNeeded;
    uint32   bytesNeeded;
    uint32   tmp;
    char   * p;
    char   * tmpTop;
    

    // Determine how many bytes are needed past page boundary
    bytesNeeded = bytes - TopBytesLeft;

    // Round up bytesNeeded to nearest page
    tmp = bytesNeeded + PageSize - 1;

    pagesNeeded = tmp / PageSize;

    if (pagesNeeded < MINPAGES) {
	pagesNeeded = MINPAGES;
    }

    // Ok grab pages needed
#ifdef __FreeBSD
    p = (char *) malloc(PageSize * pagesNeeded);
#else
    p = (char *) valloc(PageSize * pagesNeeded);
#endif

    if (!p) {
	assert(0);
    }
    
    // round top up to the nearest page to see if it will match 'p'
    tmpTop = (char *) ((uint32)( Top + (PageSize - 1) ) & ~(PageSize - 1));

    // If they are equal, the pages were contigous to top and we can return
    // (This will happen in FreeBSD since it can allocate continous pages)
    if (p == tmpTop) {
	recordPages(p, pagesNeeded);
	TopBytesLeft += (PageSize * pagesNeeded);
	return 1;
    }
    
    // else, if pages needed still satisfies this request
    // we're done, just set the new topBytesLeft, the new top,
    // and record what pages were added
    if ((PageSize * pagesNeeded) >= bytes) {
	recordPages(p, pagesNeeded);
	TopBytesLeft = (PageSize * pagesNeeded);
	Top = p;
	return 1;
    }
    

    // If all of that fails, then recalc pages needed
    // without using the TopBytesLeft. This can be
    // accomplished through recursion
    
    // Otherwise free those pages, set the number of
    // bytes available in top to be 0, do a recursive
    // call.
    // and allocate more
    free(p);
    TopBytesLeft = 0;
    
    return (addBytesTop(bytes));
    
}




/* Main public routines */

void * celmalloc(uint32 bytes)
{
    //* Debug hook */
    if (mallocDebugHook) {
	mallocDebugHook(0, bytes);
    }

    return malloc(bytes);
}

void celfree(void * memory)
{
    free(memory);
}

void * celcalloc(uint32 bytes)
{
    return calloc(1, bytes);
}


void *  celrealloc(void * mem, uint32 bytes)
{
    return realloc(mem, bytes);
}


static Bin  freeBins[256];
static Bin  fullBins[256];

static OverBin  * overBins;
static OverBin  * lastFreeOverBin;


uint32 gcBaseSet;
uint32 gcResidentSet;
Stack  gcMarkStack;


static char gcIsOn;

/*
  GC routines for right now.  Do nothing, but increase the levels
 */
void gcInit(void)
{
    gcBaseSet     = 0;
    gcResidentSet = 0;
    gcMarkStack   = StackNew(2048); // Depth of 2048

    for (i = 0; i < 256; i++) {
	freeBins[i] = null;
	fullBins[i] = null;
    }
    
    overBins = null;
    
    gcON();
}


void gcOFF(void)
{
    gcIsOn = 0;
}



void gcON(void)
{
    gcIsOn = 1;
}



void gcRun(void) 
{
    if (gcIsOn) {
	gcMarkSweep(void);
    }
}




// **It is assumed that all items on the MarkStack
// **have children and can be followed
void gcMark(void * p)
{
    Proto p;
    Proto c;
    
    
    while (!StackIsEmpty(gcMarkStack)) {

	p = (Proto) StackPull(gcMarkStack);

	childIterator = gcChildIterator(p);

	while (childIterator->next()) {
	    c = childIterator->child;

	    if (gcMarkBit(c) == 0) {

		gcMarkBit(c) = 1;

		if (gcHasChildren(c)) {
		    StackPush(gcMarkStack, M);
		}
	    }
	}

    }

}


void gcFree(Bin b, int i)
{
    // Free object
    // Put bin on free bin list? (put at top of list?)
}


void gcSweepBin(bin)
{
    uint32 i;
    
    currentBin = bin;
    
    while (1) {
	i = 0;
	while (obj = nextObj(currentBin, &i)) {
	    if ( gcMarkBit(n) == 0 ) {
		gcFree(currentBin, i);
	    }
	    else {
		gcMarkBit(obj) = 0;
	    }
	}

	currentBin = currentBin->nextBin;
	if (currentBin == 0) {
	    break;
	}
    }
}

void gcSweep(void)
{
    // Sweep bins

    for (i = 0; i < 256; i++) {
	if (freeBins[i]) {
	    gcSweepBin(freeBins[i]);
	}
    }

    for (i = 0; i < 256; i++) {
	if (fullBins[i]) {
	    gcSweepBin(fullBins[i]);
	}
    }
    
    if (overBins) {
	gcSweepOverBin(overBins);
    }
}

	    

void gcMarkSweep(void) 
{
    // Roots: local variables on stack, virtual machine registers (none), global vars (lobby)?

    // Stack references
//// stack iterator
    for R in Stack    // FROM TOP OF MEMORY DOWN or BOTTOM OF STACK UP 
	{
	    gcMarkBit(r) = 1;
	    // if object has no children
	    if (gcHasChildren(r)) {
		StackPush(gcMarkStack, r);
	    }
	}
 
    // Lobby references
//// Lobby iterator
    for R in Lobby
	{
	    gcMarkBit(r) = 1;
	    // if object has no children
	    if (gcHasChildren(r)) {
		StackPush(gcMarkStack, r);
	    }
	}

    gcMark();
    gcSweep()

//    if (memory size)
//      allocate more memory
    
}



/*
  ObjAlloc Algorithm:
*/

void * celObjAlloc(uint32 bytes, celObjAllocTypes types)
{
    void * p;
    

    if (bytes <= MAX_OBJ_SIZE) {
	// Quick look for a free bin?
	if ( ! gcBinAvail(bytes)) {
	    // If we are over our limit, then do a GC run
	    if ( ((gcResidentSet + bytes) - gcBase) > (1024 * 1024) ) {
		gc_run();
	    }
	}
	
	// This will create a bin if necessary and adjust the gcResidentSet size
	// and return a pointer to the memory
	p = gcStoreInBin(p);

	return p;
    }
    else {
	// If we are over our limit, then do a GC run
	if ( ((gcResidentSet + bytes) - gcBase) > (1024 * 1024) ) {
	    gc_run();
	}

	p = calloc(1, bytes);
	
	// This will create a bin if necessary and adjust the gcResidentSet size
	gcStoreInOverBin(p);

	return p;
    }
}



