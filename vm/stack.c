
//
// stack.c
//
// <see corresponding .h file>
// 
//
//
//


//// Right now, the only way to detect EOF is with readMSCchar

#include "alloc.h"
#include "stack.h"

// Size is in 4 byte multiples
Stack StackNew(uint32 size)
{
    Stack tmp;

    tmp = (Stack) celcalloc(sizeof(struct StackStruct));
    assert(tmp);

    tmp->bottom       = (uint32 *) celcalloc( (size << 2) );
    tmp->top          = tmp->bottom + size - 1;
    tmp->sp           = tmp->top;

    return tmp;
}


void StackFree(Stack s)
{
    celfree(s->bottom);
    celfree(s);
}


// Our stack is like the 6502 stack, the pointer is pointing at the next
// open slot for a push
void StackPush(Stack s, uint32 v)
{
    *(s->sp)  = v;
    s->sp    -= 1;
    assert(s->sp >= s->bottom);
}


uint32 StackPull (Stack s)
{

    s->sp += 1;
    assert(s->sp <= s->top);

    return( *(s->sp) );
}


uint32 StackIsEmpty(Stack s)
{
    return (s->sp == s->top);
}


