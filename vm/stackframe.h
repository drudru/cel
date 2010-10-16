
//
// stackframe.h
//
// This is the module that provides the objects for navigating the stack frames
// or activation records.
//
//
//
//

#include "cel.h"
#include "svm32.h"

#ifndef _CEL_STACKFRAME_H
#define _CEL_STACKFRAME_H 1


struct StackFrame
{
    Proto * locals;
    uint32  numLocals;
    
    Proto * args;
    uint32  numArgs;

    Proto * realTarget;
    Proto * codeProto;
    
    uint32  pc;
    uint32  fp;
    uint32  sp;
    
    uint32  isIntrinsic;
    
};


typedef struct StackFrame * StackFrame;




StackFrame StackFrameNew(VM32Cpu * cpu);

void StackFrameFree(StackFrame s);

int StackFrameNext(StackFrame sf);


#endif
