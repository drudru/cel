//
// stackframe.c
//
// <see corresponding .h file>
// 
//
//



#include "alloc.h"
#include "stackframe.h"
#include "svm32.h"
#include "runtime.h"

uint32 StackFrameValue(uint32 base, int offset)
{
    int * ptr;
    
    ptr = (int *) base;
    
    ptr += offset;
    
    return (*ptr);
}


StackFrame StackFrameNew(VM32Cpu * cpu)
{
    StackFrame tmp;

    tmp = (StackFrame) celcalloc(sizeof(struct StackFrame));
    assert(tmp);

    tmp->isIntrinsic  = 1;
    tmp->numLocals    = 0;
    tmp->pc           = cpu->pc;
    tmp->fp           = cpu->fp;
    tmp->sp           = cpu->sp;
    tmp->realTarget   = (Proto *) 0;
    tmp->codeProto    = (Proto *) 0;
    tmp->numArgs      = StackFrameValue(cpu->sp, 2);
    tmp->numArgs      = objectIntegerValue((Proto)tmp->numArgs);
    // Do this to get rid of the sentinel
    tmp->numArgs      = tmp->numArgs & 0x7f;
    tmp->args         = (Proto *)((cpu->sp + 12));

    return tmp;
}

void StackFrameFree(StackFrame s)
{
    celfree(s);
}

// Note, some of these aren't locals, they may
// just be on the stack in preparation for being
// parameters in another call
// OR we could be looking at a frame and it was
// the result of a callback

int StackFrameNext(StackFrame sf)
{
    if (sf->isIntrinsic) {
	// Reset the flag
	sf->isIntrinsic = 0;

	// Adjust the stack
	sf->sp = sf->sp + 8 + (sf->numArgs << 2);
    }
    else {
	// Detect the end
	if (sf->pc == 0) {
	    return 0;
	}

	sf->fp = StackFrameValue(sf->fp, 1);

	// Adjust the stack
	// sp + (fp, realTarget, Code) + locals + args
	sf->sp = sf->sp + 20 + (sf->numLocals << 2) + (sf->numArgs << 2);
    }
	
    // Count the locals
    sf->numLocals  = ((sf->fp - sf->sp) >> 2);
    sf->locals     = (Proto *)(sf->sp + 4);
    
    sf->pc         = StackFrameValue(sf->fp, 4);
    sf->realTarget = (Proto *) (sf->fp + 12);
    sf->codeProto  = (Proto *) (sf->fp + 8);
    
    sf->numArgs    = StackFrameValue(sf->fp, 5);
    sf->numArgs    = objectIntegerValue((Proto)sf->numArgs);
    // Do this to get rid of the sentinel
    sf->numArgs    = sf->numArgs & 0x7f;
    sf->args       = (Proto *)((sf->fp + 24));
    
    return 1;
}






