
//
// stack.h
//
// This is the module that does simple memory stream work. It is very simple
// and only here to provide an easier conversion tool for working with my objc
// code.
//
//
//
//


#ifndef _CEL_STACK_H
#define _CEL_STACK_H 1


struct StackStruct
{
    uint32 * bottom;
    uint32 * top;
    uint32 * sp;
};


typedef struct StackStruct * Stack;




// Size is in 4 byte multiples
Stack StackNew(uint32 size);

void StackFree(Stack s);

void StackPush(Stack s, uint32 v);

uint32 StackPull(Stack s);

uint32 StackIsEmpty(Stack s);



#endif
