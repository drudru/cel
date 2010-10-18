
//
// main.c
//
// <See class's '.h' file>
//
// $Id: main.c,v 1.31 2002/02/26 01:29:14 dru Exp $
//
//

#include "svm32.h"

#include "celendian.h"
#include "atom.h"
#include "runtime.h"
#include "assemble.h"
#include "compiler.h"
#include "cel2sd.h"
#include "string.h"
#include "array.h"

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


int debugParse   = 0;
int debugParseLevel = -1;
int debugCodeGen = 0;
int debugForce   = 0;
int debugTime    = 0;

extern char **environ;

#ifdef PROF
#include <sys/gmon.h>
extern void _start;
#endif

void showHelp(void)
{
    printf("Syntax: cel --help --parse --parseLevel <num> --force --codegen --noprint --time --trace --dump <module file>\n\n");
    printf("  help:       Display command line help\n");
    printf("  parse:      Display parse tree\n");
    printf("  parseLevel: Display this level x (0=all)\n");
    printf("  force:      Force the compilation of a program\n");
    printf("  codegen:    Display the generated code\n");
    printf("  trace:      Show a trace of the running code\n");
    printf("  time:       Show a trace of the running code with times (profiling)\n");
    printf("  dump:       Display the pre-compiled code module (if exists) \n");
    printf("  noprint:    Turn off the default 'print' output \n");
    
    exit(1);
}

void exitCalled(void)
{
    printf("\n\nexit called\n\n");
}

int main (int argc, char *argv[]) 
{
    
    unsigned char * p;
    unsigned char * p2;
    unsigned char * filename = "";
    int i;
    int needsCompile;

    Proto module;
    Proto environment;
    Proto arguments;
    void * mem;
    int length;
    char buff[4096];
    int sawFilename = 0;
    char dirName[256];
    char celDirName[256];
    
    

    VM32Cpu cpu;
    VM32Cpu * pcpu;


#ifdef PROF
    u_long lowPC;
    u_long highPC;
    
    // Normal glibc 2.2.x profiling uses the text segment
    // of this executable as a gauge for the data structures
    // needed for profiling. We're going to beef that up.

    lowPC  = (u_long) &_start;
    
    _mcleanup();
    
    highPC = (lowPC + (MAXARCS * 50));
    monstartup(lowPC, highPC);
#endif
    

    // Unbuffer IO for easier debugging
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    
    

    // Setup the search path from the path of the cel executable
    if (argv[0][0] != 'c') {
	// Extract the path
	p2 = argv[0];
#ifdef unix
	p = strrchr(p2, '/');
#else
	p = strrchr(p2, '\\');
#endif
	assert(p);
	SearchPaths[0] = celDirName;
	memcpy(celDirName, p2, (p - p2));
	celDirName[(p - p2)] = 0;
	strcat(celDirName, "/kits");
    }



    for (i = 1; i < argc; i++) {
       if (argv[i][0] == '-') {
         if (strcmp(argv[i], "--help") == 0) {
           showHelp();
         } else 
         if (strcmp(argv[i], "--trace") == 0) {
	   debugTrace = 1;
           printf("debugTrace ON\n");
         } else 
         if (strcmp(argv[i], "--time") == 0) {
	   debugTime = 1;
           printf("debugTime ON\n");
	   debugTrace = 1;
           printf("debugTrace ON\n");
         } else 
         if (strcmp(argv[i], "--dump") == 0) {
	   debugDump = 1;
           printf("debugDump ON\n");
         } else
         if (strcmp(argv[i], "--force") == 0) {
	   debugForce = 1;
           printf("debugForce ON\n");
         } else
         if (strcmp(argv[i], "--parse") == 0) {
	   debugParse = 1;
           printf("debugParse ON\n");
         } else
         if (strcmp(argv[i], "--noprint") == 0) {
	   noPrint = 1;
         } else
         if (strcmp(argv[i], "--parseLevel") == 0) {
	     i++;
	     if (i > argc) {
		 printf("Missing argument to parseLevel\n");
		 exit(1);
	     }
	     if (!isdigit(argv[i][0])) {
		 printf("Improper argument to parseLevel\n");
		 exit(1);
	     }
	     debugParseLevel = atoi(argv[i]);
	     printf("debugParseLevel = %d ON\n", debugParseLevel);
         } else
         if (strcmp(argv[i], "--codegen") == 0) {
	   debugCodeGen = 1;
           printf("debugCodeGen ON\n");
         }
       } else {
           if (sawFilename != 1) {
	     filename = argv[i];
	     sawFilename = 1;
           }
       }
    }

    // Load the module
    if (argc == 1) {
	printf("Dru get the command line eval going\n\n");
        showHelp();
    }

    // If filename has '.cel' suffix, then remove that
    p = strstr(filename, ".cel");
    if (NULL != p) {
      // If the suffix is present, remove it
      *p = 0;
    }

    // If filename has a '/' then copy all of the filename
    // up to the last '/' as the dirName
    p = celRevStrStr(filename, strlen(filename), "/", 1);
    if (p != NULL) {
	memcpy(dirName, filename, (p - filename));
	dirName[p-filename] = 0;
	filename = p + 1;
    } else {
	dirName[0] = 0;
    }



    ///////////////////////////////////////////////////////////////////////////
    // Start the runtime



    // Setup our virtual CPU
    pcpu = &cpu;
    init(pcpu);

    // Setup our runtime
    // This sets up the atoms, globals, and internal intrinsics
    celInit(pcpu);

    p = UniqueString("car");
    p = UniqueString("car");


    // Check if the module needs to be compiled to disk
    needsCompile = checkCompiled(filename);


    if (needsCompile || debugForce) {
	// Load the module, note the cel objects
	GCOFF();
	module = compile(loadModuleStream(filename), debugParse, debugCodeGen);
	////DRUDRU - need to free that memstream
	mem = cel2sd(module, &length);
	GCON();
	
	if (writeModule(filename, mem, length)) {
	  printf("Couldn't write compiled module [%s]\n\n", filename);
	  exit(1);
	} 
	// The module is all cel objects, it will get free'd automatically
	// The memory from the cel2sd must be explicitly freed
	// (Although, we could make it a blob and let the collector deal
	//  with it)
	celfree(mem);
    }
    

    // Prepare the module for running (sets up first activation record)
    // args(filename, donotRunflag) 
    doCelInit(filename, pcpu, 0);


    // Setup the Environment
    i = 0;

    GCOFF();
    
    environment = objectNew(0);
    arguments = createArray(nil);
    objectSetSlot(Capsule, stringToAtom("environment"), PROTO_ACTION_GET, (uint32) environment);
    objectSetSlot(Capsule, stringToAtom("arguments"), PROTO_ACTION_GET, (uint32) arguments);
    
    while (1) {
	if (environ[i] == NULL) {
	    break;
	}

	strcpy(buff, environ[i]);
	
	p = strchr(buff, '=');
	*p= 0;
	p++;
	
	objectSetSlot(environment, stringToAtom(buff), PROTO_ACTION_GET, (uint32) stringToAtom(p));
	i++;
    }
    
    for (i = 0; i < argc; i++) {
	//printf("%d - %s\n", i, argv[i]);
	addToArray(arguments, (Proto) stringToAtom(argv[i]));
    }

    //objectDump(arguments);
    
    if (debugTrace) {
      // Turn on debug mode
      runtimeActivateTrace = 1;
    }

    GCON();
    

    // Run the code straight
    VMRun (pcpu, 1);
    
    return 0;
}

