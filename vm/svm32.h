//
// svm32.h
//
// This is the description of the functions for the Stack-VM 32 bit
// 
//
// $Id: svm32.h,v 1.12 2002/02/28 19:20:53 dru Exp $
//
//
//
//


#ifndef H_CELSVM32
#define H_CELSVM32

struct _vm32cpu {
	unsigned int pc;	/* Program Counter */
	unsigned int sp;	/* Stack Pointer */
	unsigned int fp;	/* Frame Pointer */
	unsigned int haltflag;	/* Halt Flag :-) */
	// The following two are for branches
	unsigned int trueObj;	/* Value that references true obj */
	unsigned int falseObj;	/* Value that references false obj */
	unsigned int stackBottom;	/* Stack bottom, if we go past this, we're toast */
	unsigned int stackTop;          /* Stack top, if we go past this, we're toast */
	void (*breakHook) (struct _vm32cpu *);	/* Break Handler */
	void (*invalidHook)(struct _vm32cpu *);	/* Invalid Instruction Handler */
	void (*argCountHook)(struct _vm32cpu *);	/* Invalid message argument count hook */
	unsigned int (*pushHook)(struct _vm32cpu *, unsigned int);	/* Hook for special pushImmediate handling */
	void (*activateHook)(void);	/* Hook for activateSlot */
};


typedef struct _vm32cpu VM32Cpu;


inline void push(VM32Cpu * cpu, unsigned int v);
inline unsigned int pull(VM32Cpu * cpu);
inline unsigned int stackAt(VM32Cpu * cpu, unsigned int i);
void setStackAt(VM32Cpu * cpu, unsigned int i, unsigned int data);

inline unsigned int getFrameAt(VM32Cpu * cpu, int i);
inline unsigned int getFPRelativeAt(unsigned int fp, int i);

inline unsigned int setFrameAt(VM32Cpu * cpu, int pos, unsigned int value);
inline unsigned int setFPRelativeAt(unsigned int fp, int pos, unsigned int value);


void init (VM32Cpu * cpu);

void VMReset (VM32Cpu * );
void VMRun (VM32Cpu *, unsigned int );
void VMSetupCall (VM32Cpu * , unsigned int , unsigned int );
void VMSetupCallWithArgs (VM32Cpu * cpu, unsigned int obj, unsigned int mesg, unsigned int num, unsigned int * pArray);
int singleStep (VM32Cpu * );
int halt (VM32Cpu *);


// User level routines 
inline void VMReturn(VM32Cpu * cpu, unsigned int returnValue, unsigned int moveCount);

// Debugger Support
int disassembleCpu(VM32Cpu * cpu, char * * opcode,  int * length);


#endif
