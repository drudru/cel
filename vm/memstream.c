
//
// memstream.c
//
// <see corresponding .h file>
// 
//
//


//// Right now, the only way to detect EOF is with readMSCchar

#include "alloc.h"
#include "celendian.h"
#include "memstream.h"

MemStream NewMemStream(char * start, uint32 size)
{
    MemStream tmp;

    tmp = (MemStream) celcalloc(sizeof(struct MemStreamStruct));
    tmp->start  = start;
    tmp->size   = size;
    tmp->cursor = 0;
    tmp->allocated = 0;
    tmp->lineNumber   = 1;
    tmp->columnNumber = 0;

    return tmp;
}


void freeMemStream(MemStream ms)
{
    celfree(ms);
}

// 0 is the first byte
int seekMS(MemStream ms, uint32 where)
{
    ms->cursor = where;
    return 0;
}


int seekMSRelative(MemStream ms, int where)
{
    ms->cursor += where;
    return ms->cursor;
}

int getMSSize(MemStream ms)
{
    return ms->size;
}


int positionMS(MemStream ms)
{
    return ms->cursor;
}

int MSlineNumber(MemStream ms)
{
    return ms->lineNumber;
}

int MScolumnNumber(MemStream ms)
{
    return ms->columnNumber;
}

void getMSLineCol(MemStream ms, uint32 pos, uint32 * line, uint32 * col)
{
  int i = 0;
  char c;
  int cl, ln;
  

  cl = ln = 0;

  while (1) {
    c = ms->start[i];
    if (c == '\n') {
      ln += 1;
      cl  = -1;
    }

    i++;
    cl += 1;
    if (i == pos) {
	break;
    }
  }

  *col  = (uint32) cl;
  *line = (uint32) ln;
  
  return;
}

void getMSLine(MemStream ms, uint32 line, char * buff)
{
  int i = 0;
  int l = 0;
  char c;
  int start, end;

  start = i;
  
  while (1) {
    c = ms->start[i];

    if (c == '\n') {
      l++;
  
      if (l == (line + 1)) {
	  end = i + 1;
	  break;
      }

      start = i + 1;
    }
    
    i++;
    if (i >= ms->size) {
        end = i;
	break;
    }
  }

  // Copy data into buff
  memcpy(buff, (ms->start + start), (end - start));
  // Insure that the buff is terminated
  buff[(end-start)] = 0;

  return;
}

int readMSSizeInto(MemStream ms, uint32 size, char * into)
{
    // Copy the memory
    memcpy(into, (ms->start + ms->cursor), size);
    
    // Position the cursor
    ms->cursor += size;
    return size;
}

int readMSCInt(MemStream ms)
{
    int i;

    readMSSizeInto(ms, 4, (void *)&i);
    i = std2hostInteger(i);
    return i;
}

int readMSCChar(MemStream ms)
{
    int i;

    if (ms->cursor >= ms->size) {
	return EOF;
    }
    i = (unsigned int) ms->start[ms->cursor];

    ms->cursor++;

    ms->columnNumber++;
    
    if (i == '\n') {
	ms->lineNumber++;
	ms->columnNumber = 0;
    }
    
    return i;
}



int addMSSize(MemStream ms, uint32 size)
{
    uint32 bytes;
    
    if ((ms->cursor + size) > ms->allocated) {
	bytes = (ms->cursor + size) - ms->allocated;
	if (bytes < 8192) {
	    bytes = 8192;
	}
	
	ms->start = (char *) celrealloc(ms->start,  (ms->allocated + bytes) );
	ms->allocated = (ms->allocated + bytes);
    }

    if ((ms->cursor + size) > ms->size) {
	ms->size = ms->cursor + size;
    }


    return 1;
}

	

// Write routines

int writeMSCSizeFrom(MemStream ms, uint32 size, char * from)
{

    addMSSize(ms, size);
    
    // Copy the memory
    memcpy((ms->start + ms->cursor), from, size);
    
    // Position the cursor
    ms->cursor += size;
    
    return size;
}

int writeMSCInt(MemStream ms, int i)
{
    int j;

    addMSSize(ms, 4);

    j = host2stdInteger(i);
    writeMSCSizeFrom(ms, 4, (void *)&j);
    return 4;
}

int writeMSCChar(MemStream ms, char c)
{
    addMSSize(ms, 1);

    ms->start[ms->cursor] = c;
    
    ms->cursor++;

    return 1;
}
