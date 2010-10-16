
//
// celdump.c
//
// <See class's '.h' file>
//
// $Id: celdump.c,v 1.4 2001/01/05 00:58:54 dru Exp $
//
//


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "svm32.h"

#include "celendian.h"
#include "atom.h"
#include "runtime.h"
#include "assemble.h"
#include "sd2cel.h"

#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int debugParse;
int debugParseLevel = -1;
int debugCodeGen;
int debugForce;



int main (int argc, char *argv[]) 
{
    

    Proto realRoot;
    Proto root;
    Proto start;
    
    Proto tmpObj;
    
    unsigned char * p;
    unsigned char * module;
    int i;

    VM32Cpu cpu;
    VM32Cpu * pcpu;



    // Setup the memory and the GC


    // Setup our virtual CPU
    pcpu = &cpu;
    init(pcpu);
    reset(pcpu);




    // Setup our runtime
    celInit(0, pcpu);

    
    for (i = 0; i < argc; i++) {
       if (argv[i][0] == '-') {
         if (strcmp(argv[i], "--trace") == 0) {
	   debugTrace = 1;
           printf("debugTrace ON\n");
         } else 
         if (strcmp(argv[i], "--dump") == 0) {
	   debugDump = 1;
           printf("debugDump ON\n");
         }
       } else {
         module = argv[i];
       }
    }

    // Load the module
    if (argc == 1) {
	printf("Syntax: celdump <module file>\n\n");
	exit(1);
    }



    root = loadModule(module);

    if (nil == root) {
      printf("%s", sd2celError());
    }
    
    objectDump(root);
    
    
    return 0;
}



