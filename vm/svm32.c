
/* REview the codes and fill out the jumps, and decide on signed 
   operations. Imagine that this is running interupt code or serious
   OO code 
 */
#include "alloc.h"
#include "celendian.h"
#include "svm32.h"

/* Our stack is like the 6502 stack, the pointer is pointing at the next
   open slot for a push */
inline void push(VM32Cpu * cpu, unsigned int v) {
    int * ptr;

    ptr = (int *) cpu->sp;
    *ptr = v;
    cpu->sp -= 4;
    assert(cpu->sp >= cpu->stackBottom);
}

inline unsigned int pull(VM32Cpu * cpu) {
    int * ptr;

    cpu->sp += 4;
    ptr = (int *) cpu->sp;
    assert(cpu->sp < cpu->stackTop);
    return(*ptr);
}

// Return the stack object at a certain point
// 0 is the top, 1 is under it, etc...
unsigned int stackAt(VM32Cpu * cpu, unsigned int i) 
{
    int * ptr;

    ptr = (int *) cpu->sp;
    ptr++;			// Increment to point to top of stack
    
    return ptr[i];
}

// Return the stack object at a certain point
// 0 is the top, 1 is under it, etc...
void setStackAt(VM32Cpu * cpu, unsigned int i, unsigned int data) 
{
    int * ptr;

    ptr = (int *) cpu->sp;
    ptr++;			// Increment to point to top of stack
    
    ptr[i] = data;
}

// Return the stack object at a certain point relative to the fp
// 0 is the top, 1 is under it, etc...
inline unsigned int getFrameAt(VM32Cpu * cpu, int i)
{
    int * ptr;

    ptr = (int *) cpu->fp;
    
    return ptr[i];
}

// Return the stack object at a certain point relative to the fp
// 0 is the top, 1 is under it, etc...
inline unsigned int getFPRelativeAt(unsigned int fp, int i) 
{
    int * ptr;

    ptr = (int *) fp;
    
    return ptr[i];
}

// Set the object in the frame relative position
// 0 is the top, 1 is under it, etc...
inline unsigned int setFrameAt(VM32Cpu * cpu, int pos, unsigned int value){
    int * ptr;

    ptr = (int *) cpu->fp;
    
    return (ptr[pos] = value);
}

// Return the stack object at a certain point relative to the fp
// 0 is the top, 1 is under it, etc...
inline unsigned int setFPRelativeAt(unsigned int fp, int pos, unsigned int value) 
{
    int * ptr;

    ptr = (int *) fp;
    
    return (ptr[pos] = value);
}


/////////////////////////////////////////////////////////////////////
//
//
//  I N S T R U C T I O N   H A N D L I N G   R O U T I N E S
//
//
/////////////////////////////////////////////////////////////////////



inline void DoBreak(VM32Cpu * cpu) {
	cpu->haltflag = 1;
	cpu->breakHook(cpu);
}

inline void DoNothing(VM32Cpu * cpu) {
    cpu->pc++;
}


// this calls activateSlot
inline void DoCall (VM32Cpu * cpu)
{
    cpu->pc++;			// Increment the program counter
    push(cpu,cpu->pc);		// Push the PC so upon return it executes the next
    cpu->activateHook();	// Call activateSlot
}


inline void DoPushZero(VM32Cpu * cpu) {
    cpu->pc++;
    push(cpu, 0);
}

inline void DoPushImmediateNative(VM32Cpu * cpu) {
    int * ip;
    unsigned int j;
    
    cpu->pc++;
    ip = (int *) cpu->pc;

    j = std2hostInteger(*ip);
    /* Convert to little endian */

    // If this is a block proto, we need to do a callback
    if (cpu->pushHook) {
      j = cpu->pushHook(cpu, j);
    }
    push(cpu, j);

    cpu->pc += 4;
}

inline void DoPushSelf(VM32Cpu * cpu) {
    uint32 i;
    
    cpu->pc++;

    // Retrieve the self (the code-proto pointer)
    i = getFrameAt(cpu, 2);

    push(cpu, i);
}

inline void DoReturn(VM32Cpu * cpu) {
    int * ip;
    unsigned int j;
    unsigned int returnVal;
    int * stack;
    

    // Get the operand
    // The operand specifies how much stack to unwind
    // (all of the parameters used to setup the call by
    //  the caller)
    cpu->pc++;
    ip = (int *) cpu->pc;
    j = std2hostInteger(*ip);
    cpu->pc += 4;

    // Now place the return value in position and move the pc
    // and adjust the stack to that position

    stack = (int *) cpu->sp;
    stack++;			// Point at the return value
    returnVal = *stack;
    stack++;			// Point at the preserved code-proto ptr
    stack++;			// Point at the preserved parent-local
    stack++;			// Point at the return program counter
    cpu->pc = *stack;		// Set the program counter
    // A return PC of 0x01 is a sentinel for a C callback
    // Therefore, this return should halt the VMRun loop and do a C level
    // return
    stack++;			// Point at the arg count
    if (*stack > 0x200) {
       cpu->haltflag = 1;
    }
    stack--;			// Point at the arg count
    
    stack = stack + j;		// Now point at where the return value should go
    *stack = returnVal;		// Put the value there
    stack--;			// Move the stack pointer down

    // Now set the vm stack pointer
    cpu->sp = (unsigned int) stack;
}


inline void DoPop(VM32Cpu * cpu) {
    cpu->pc++;			// Increment the program counter
    pull(cpu);
}


inline void DoSwap(VM32Cpu * cpu) {
    unsigned int i;
    int * stack;
    

    cpu->pc++;

    // Swap the two top most items on the stack
    stack = (int *) cpu->sp;
    stack++;			// Point at the top of stack value
    i = *stack;
    *stack = *(stack + 1);
    stack++;
    *stack = i;
}

inline void DoSetupFrame(VM32Cpu * cpu) {

    unsigned int * ip;
    unsigned int j;
    
    cpu->pc++;

    // Get the operand
    ip = (int *) cpu->pc;
    j = std2hostInteger(*ip);

    // Push the previous frame pointer
    push(cpu, cpu->fp);

    // Set the fp to the current stack pointer
    cpu->fp = cpu->sp;

    while (j) {
      // push nil's for the locals
      push(cpu, 0);
      j--;
    }
    cpu->pc += 4;
    
}

inline void DoRestoreFrame(VM32Cpu * cpu) {
    unsigned int returnVal;
    
    cpu->pc++;

    // Get the return value
    returnVal = pull(cpu);

    cpu->sp = cpu->fp;		// Restore the sp
    cpu->fp = pull(cpu);	// Restore the fp from there
    push(cpu, returnVal);	// Now put the returnVal in preparation for a return
}


inline void DoBranch(VM32Cpu * cpu) {
    unsigned int * ip;
    int j;
    
    cpu->pc++;

    // Get the operand
    ip = (int *) cpu->pc;
    j = std2hostInteger(*ip);

    // Increment the PC past the operand
    cpu->pc += 4;

    // Offset the PC by the operand
    cpu->pc += j;
}

inline void DoBranchIfTrue(VM32Cpu * cpu) {
    unsigned int * ip;
    int j;
    unsigned int val;
    
    cpu->pc++;

    // Get the operand
    ip = (int *) cpu->pc;
    j = std2hostInteger(*ip);

    // Increment the PC past the operand
    cpu->pc += 4;

    // Get the value on the stack
    val = pull(cpu);

    if (val == cpu->trueObj) {
	// Offset the PC by the operand
	cpu->pc += j;
    }
}

inline void DoBranchIfFalse(VM32Cpu * cpu) {
    unsigned int * ip;
    int j;
    unsigned int val;
    
    cpu->pc++;

    // Get the operand
    ip = (int *) cpu->pc;
    j = std2hostInteger(*ip);

    // Increment the PC past the operand
    cpu->pc += 4;

    // Get the value on the stack
    val = pull(cpu);

    if (val == cpu->falseObj) {
	// Offset the PC by the operand
	cpu->pc += j;
    }
}


inline void CallNative(VM32Cpu * cpu) {

//    unsigned int esp;
    unsigned int j;
    int * ip;
    

    
    cpu->pc += 1;		/* Increment the program counter */
    ip = (int *) cpu->pc;
    j = std2hostInteger(*ip);			/* Get the function to call */
    

    cpu->sp += 4;		/* The Intel uses a stack pointer that points
				   to the last thing pushed so we adjust to that */
    
    /* Our fp is pointing to our frame */
    /* We preserve our esp there */
    /* We change the esp to be the VM Stack */
    /* We call syscall */
        /* it preserves the fp (ebx) and allocates space on its stack */
        /* it pulls the parms off of the stack*/
        /* when it's done, it does a ret*/
        /* the return pc was the last value pushed */
    /* */

    /*
    asm (
	"movl %%esp,%0; movl %1,%%esp; call %2; movl %0, %%esp"
	:: "m" (esp), "m" (cpu->sp), "m" (j) );
    */

    /* Restore the real stack */
    
    cpu->pc += 4;
    cpu->sp -= 4;		/* Restore the stack pointer to our VM style */
    
    /* Note that the result is in native format */
}


// Check the Argument count
// operand: count that arguments should be
// ??This should be the first instruction on
// a method before setupFrame??
inline void DoCheckArgCount(VM32Cpu * cpu) {
    int * ip;
    int count;
    int j;
    
    
    // Move to the operand 
    cpu->pc++;
    ip = (int *) cpu->pc;

    j = std2hostInteger(*ip);
    cpu->pc += 4;

    count = stackAt(cpu, 2);

    if (count != j) {
	cpu->haltflag = 1;
	cpu->argCountHook(cpu);
    }
}


inline void DoInvalidInstruction(VM32Cpu * cpu) {
	cpu->haltflag = 1;
	cpu->invalidHook(cpu);
}


/* Default Break Handler for breakHook */
void DefaultBreak(VM32Cpu * cpu) {
	cpu->haltflag = 1;
//	printf("Break hit at PC:[%08x]\n", cpu->pc);
}
	

void DefaultInvalidInstruction(VM32Cpu * cpu) {
	unsigned char * p;

	cpu->haltflag = 1;
	p = (char *) cpu->pc;
	
//	printf("Invalid Instruction [%02x] hit at PC:[%08x]\n", *p, cpu->pc);
}
	
void DefaultInvalidArgCount(VM32Cpu * cpu) {
	unsigned char * p;

	cpu->haltflag = 1;
	p = (char *) cpu->pc;
	
//	printf("Invalid Argument Count [%02x] hit at PC:[%08x]\n", *p, cpu->pc);
}
	


void init (VM32Cpu * cpu) 
{
	VMReset(cpu);
	cpu->breakHook    = DefaultBreak;
	cpu->invalidHook  = DefaultInvalidInstruction;
	cpu->argCountHook = DefaultInvalidArgCount;
	cpu->pushHook     = NULL;
	cpu->activateHook = NULL;
}

void VMReset (VM32Cpu * cpu) 
{
	cpu->pc = cpu->sp = cpu->fp = 0;
	cpu->haltflag = 0;
}

void VMRun (VM32Cpu * cpu, unsigned int startWithCall) 
{
    // This is necessary for some callback systems. We
    // Sometimes we just want to setup the stack and 
    // have a call go. If we didn't have this, we
    // would have to setup a place in memory for a 
    // call opcode followed by a return
    if (startWithCall) {
        // Notice that we don't increment the program counter
        cpu->activateHook();
    }
    while (!cpu->haltflag) {
	singleStep(cpu);
    }

    // Reset the haltflag
    cpu->haltflag = 0;
}

int singleStep (VM32Cpu * cpu)
{
    unsigned char inst;
    unsigned char * ptr;
    
    /* Get an instruction */
    ptr = (char *) cpu->pc;
    inst = *ptr;
    
    switch (inst) {
	/* Standard Op Codes */
	case 0x00: DoBreak(cpu);
	    break;
	case 0x01: DoNothing(cpu);
	    break;
	case 0x02: DoCall(cpu);
	    break;
	case 0x03: DoPushZero(cpu);
	    break;
	case 0x04: DoPushImmediateNative(cpu);
	    break;
	case 0x05: DoReturn(cpu);
	    break;
	case 0x06: DoPop(cpu);
	    break;
	case 0x07: DoSwap(cpu);
	    break;
	case 0x08: DoSetupFrame(cpu);
	    break;
	case 0x09: DoRestoreFrame(cpu);
	    break;
	case 0x0a: CallNative(cpu);
	    break;
	case 0x0b: DoPushSelf(cpu);
	    break;
	case 0x0c: DoCheckArgCount(cpu);
	    break;
	case 0x0d: DoBranch(cpu);
	    break;
	case 0x0e: DoBranchIfTrue(cpu);
	    break;
	case 0x0f: DoBranchIfFalse(cpu);
	    break;
	default:
	    DoInvalidInstruction(cpu);
	    break;
    }
    return 0;
}

int halt (VM32Cpu * cpu) 
{
	cpu->haltflag = 1;
	return 0;
}



//  These are for non-aqua-code routines which perform actions 
//  without having setup an activation frame/record. However, they need to
//  return

inline void VMReturn(VM32Cpu * cpu, unsigned int returnValue, unsigned int moveCount)
{
    int * stack;
    
    // Now place the return value in position and move the pc
    // and adjust the stack to that position
    
    stack = (int *) cpu->sp;
//    stack++;			// Point at the preserved selfReg
//    cpu->selfReg = *stack;	// Restore the selfReg
    stack++;			// Point at the return program counter
    cpu->pc = *stack;		// Set the program counter
    
    stack = stack + moveCount;	// Now point at where the return value should go
    *stack = returnValue;	// Put the value there
    stack--;			// Move the stack pointer down
    
    // Now set the vm stack pointer
    cpu->sp = (unsigned int) stack;
}

// Setup a call 
void VMSetupCall (VM32Cpu * cpu, unsigned int obj, unsigned int mesg) 
{
    // This 'primes the pump' for a call to activateSlot.
    // We push the context and then activateSlot, this activates the _Start
    // method. This is a 'passthru'. As such, an activate to it will just
    // setup the FP and Parent pointer (to the target) and then set the
    // PC (Program Counter) to the proper code routine.
    // Then the cpu will have to do more..


    // Push the parent-local (true-target)
    //push(pcpu, (uint32) nil);
    // Push the code-proto pointer
    //push(pcpu, (uint32) nil);

    // Push to preserve FP
    //push(pcpu, cpu->fp);

    // Setup the fp
    // pcpu->fp = pcpu->sp;

    // The FP gets preserved with the setupFrame that will occur
    // in the new activateion
    // The PC gets preserved on the stack below

    push(cpu, mesg);
    push(cpu, obj);
    // Note: this is special
    push(cpu,  0x419 );       // The Argument count
    // (((0x80 | 0x3) << 3) | 0x1)
    // Essentially an indicator/sentinel that 
    // the DoReturn should end
    push(cpu, cpu->pc);
}


// Setup a call 
void VMSetupCallWithArgs (VM32Cpu * cpu, unsigned int obj, unsigned int mesg, unsigned int num, unsigned int * pArray) 
{
    int i;
    // See above for the big description

    i = num;

    while (i) {
       i--;
       push(cpu, pArray[i]);
    }

    push(cpu, mesg);
    push(cpu, obj);
    // Note: mind the sentinel, this is a special frame
    // see above.
    push(cpu,  ((((3 + num) | 0x80) << 3) | 1) );
    push(cpu, cpu->pc);
}


/* This returns a flag indicating that the opcode was a good
   opcode. If it is good, the length (number of bytes for operand)
   and a character string pointing to the opcode will be returned
   (I should return a proto, but for now, we'll leave it at this)
*/

int disassembleCpu(VM32Cpu * cpu, char * * opcode,  int * length)
{
    unsigned char inst;
    unsigned char * ptr;
    
    /* Get an instruction */
    ptr = (char *) cpu->pc;
    inst = *ptr;
    
    switch (inst) {
	/* Standard Op Codes */
	case 0x00:
	    *opcode = "brk";
            *length = 0;
            return 1; 
	    break;
	case 0x01:
	    *opcode = "noop";
            *length = 0;
            return 1; 
	    break;
	case 0x02:
	    *opcode = "call";
            *length = 0;
            return 1; 
	    break;
	case 0x03:
	    *opcode = "pushZero";
            *length = 0;
            return 1; 
	    break;
	case 0x04:
	    *opcode = "pushImmediate";
            *length = 4;
            return 1; 
	    break;
	case 0x05:
	    *opcode = "return";
            *length = 4;
            return 1; 
	    break;
	case 0x06:
	    *opcode = "pop";
            *length = 0;
            return 1; 
	    break;
	case 0x07:
	    *opcode = "swap";
            *length = 0;
            return 1; 
	    break;
	case 0x08:
	    *opcode = "setupFrame";
            *length = 4;
            return 1; 
	    break;
	case 0x09:
	    *opcode = "restoreFrame";
            *length = 0;
            return 1; 
	    break;
	case 0x0a:
	    *opcode = "callNative";
            *length = 4;
            return 1; 
	    break;
	case 0x0b:
	    *opcode = "pushSelf";
            *length = 0;
            return 1; 
	    break;
	case 0x0c:
	    *opcode = "argCount";
            *length = 4;
            return 1; 
	    break;
	case 0x0d:
	    *opcode = "branch";
            *length = 4;
            return 1; 
	    break;
	case 0x0e:
	    *opcode = "branchIfTrue";
            *length = 4;
            return 1; 
	    break;
	case 0x0f:
	    *opcode = "branchIfFalse";
            *length = 4;
            return 1; 
	    break;
	default:
	    *opcode = "???";
            *length = 0;
            return 1; 
	    break;
    }
    return 0;
}


