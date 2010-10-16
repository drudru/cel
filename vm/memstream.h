
//
// memstream.h
//
// This is the module that does simple memory stream work. It is very simple
// and only here to provide an easier conversion tool for working with my objc
// code.
//
//
//
//


#ifndef _CEL_MEMSTREAM_H
#define _CEL_MEMSTREAM_H 1


struct MemStreamStruct
{
    char * start;
    uint32 size;
    uint32 cursor;
    uint32 allocated;		// memory allocated for writes
    // These will only work if you don't do any seeks! Of course, even the compiler
    // will fail this. I need to make this much more solid.
    uint32 lineNumber;
    uint32 columnNumber;
};


typedef struct MemStreamStruct * MemStream;

#ifndef EOF
#define EOF  (-1)
#endif

MemStream NewMemStream(char * start, uint32 size);

// Used by the compiler - these will have valid line number and column
// seekMS, seekMSRelative, positionMS, MSLineNumber, MScolumnNumber,
// readMSCChar

void freeMemStream(MemStream ms);

// 0 is the first byte
int seekMS(MemStream ms, uint32 where);
int seekMSRelative(MemStream ms, int where);
int getMSSize(MemStream ms);

int positionMS(MemStream ms);
int MSlineNumber(MemStream ms);
int MScolumnNumber(MemStream ms);

void getMSLineCol(MemStream ms, uint32 pos, uint32 * line, uint32 * col);
void getMSLine(MemStream ms, uint32 line, char * buff);

int readMSSizeInto(MemStream ms, uint32 size, char * into);
int readMSCInt(MemStream ms);
int readMSCChar(MemStream ms);

int addMSCSize(MemStream ms, int i);

int writeMSCSizeFrom(MemStream ms, uint32 size, char * from);
int writeMSCInt(MemStream ms, int i);
int writeMSCChar(MemStream ms, char c);




#endif
