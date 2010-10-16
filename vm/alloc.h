

/* 

11/1/2001 - An even newer, different version of malloc fo cel

8/25/98 - This is a completely new version of malloc for cel - celmalloc

*/



#ifndef H_CELALLOC
#define H_CELALLOC



#include <stddef.h>   /* for size_t */
#include <stdlib.h>

#include <assert.h>


/* Debug Message Handler type= 0: malloc, type=1 free */ 
extern int (*mallocDebugHook) (int type, int mem);



#ifndef uint32
#define uint32 unsigned long
#endif

#ifndef int32
#define int32 long
#endif

#ifndef uint16
#define uint16 unsigned short
#endif

#define nil   (void *)0



/* Use Duff's device for good zeroing/copying performance. */

#define MALLOC_ZERO(charp, nbytes)                                            \
do {                                                                          \
  uint32* mzp = (uint32*)(charp);                           \
  long mctmp = (nbytes)/sizeof(uint32), mcn;                         \
  if (mctmp < 8) mcn = 0; else { mcn = (mctmp-1)/8; mctmp %= 8; }             \
  switch (mctmp) {                                                            \
    case 0: for(;;) { *mzp++ = 0;                                             \
    case 7:           *mzp++ = 0;                                             \
    case 6:           *mzp++ = 0;                                             \
    case 5:           *mzp++ = 0;                                             \
    case 4:           *mzp++ = 0;                                             \
    case 3:           *mzp++ = 0;                                             \
    case 2:           *mzp++ = 0;                                             \
    case 1:           *mzp++ = 0; if(mcn <= 0) break; mcn--; }                \
  }                                                                           \
} while(0)

#define MALLOC_COPY(dest,src,nbytes)                                          \
do {                                                                          \
  uint32* mcsrc = (uint32*) src;                            \
  uint32* mcdst = (uint32*) dest;                           \
  long mctmp = (nbytes)/sizeof(uint32), mcn;                         \
  if (mctmp < 8) mcn = 0; else { mcn = (mctmp-1)/8; mctmp %= 8; }             \
  switch (mctmp) {                                                            \
    case 0: for(;;) { *mcdst++ = *mcsrc++;                                    \
    case 7:           *mcdst++ = *mcsrc++;                                    \
    case 6:           *mcdst++ = *mcsrc++;                                    \
    case 5:           *mcdst++ = *mcsrc++;                                    \
    case 4:           *mcdst++ = *mcsrc++;                                    \
    case 3:           *mcdst++ = *mcsrc++;                                    \
    case 2:           *mcdst++ = *mcsrc++;                                    \
    case 1:           *mcdst++ = *mcsrc++; if(mcn <= 0) break; mcn--; }       \
  }                                                                           \
} while(0)




/* Public routines */

void *  celmalloc(uint32);
void    celfree(void *);
void *  celcalloc(uint32);
void *  celrealloc(void *, uint32);
void *  celmalloc(uint32);

void gcInit(void);

#if 0
#define GCON()    gcON();printf("GCON: %s:%d = %d\n", __FUNCTION__ , __LINE__,gcGetIsOn());
#define GCOFF()   gcOFF();printf("GCOFF: %s:%d = %d\n", __FUNCTION__ ,__LINE__,gcGetIsOn());
#else
#define GCON()    gcON();
#define GCOFF()   gcOFF();
#endif

void gcOFF(void);
void gcON(void);
char gcGetIsOn(void);

void gcRun(void);


void * celObjAlloc(uint32 sizeBytes);



#endif 
