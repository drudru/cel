
//
// compmain.c
//
// <See class's '.h' file>
//
// $Id: compmain.c,v 1.8 2001/01/05 00:58:54 dru Exp $
//
//


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "svm32.h"

#include "atom.h"
#include "runtime.h"
#include "assemble.h"
#include "cel2sd.h"
#include "compiler.h"
#include "string.h"


#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/mman.h>



MemStream loadModuleStream(char * fileName);
int  writeModule(char * fileName, void * mem, int length);



int main (int argc, char *argv[]) 
{
    unsigned char * p;

    VM32Cpu cpu;
    VM32Cpu * pcpu;
    Proto module;
    int   length;
    void * mem;
    int i;
    char * filename;
    int debugParse   = 0;
    int debugCodeGen = 0;

    
    
    // Setup the memory and the GC

    // Setup our virtual CPU
    pcpu = &cpu;
    init(pcpu);
    reset(pcpu);

    // Setup our runtime
    celInit(0, pcpu);

    p = UniqueString("car");
    p = UniqueString("car");
    
    for (i = 0; i < argc; i++) {
       if (argv[i][0] == '-') {
         if (strcmp(argv[i], "--parse") == 0) {
	   debugParse = 1;
           printf("debugParse ON\n");
         } else 
         if (strcmp(argv[i], "--codegen") == 0) {
	   debugCodeGen = 1;
           printf("debugCodeGen ON\n");
         }
       } else {
         filename = argv[i];
       }
    }

    // Load the module
    if (argc == 1) {
	printf("Syntax: compile --parse --codegen <source file>\n\n");
	exit(1);
    }


    // Load the module
    module = compile(loadModuleStream(filename), debugParse, debugCodeGen);
    mem = cel2sd(module, &length);
    writeModule(filename, mem, length);

    return 0;
}


int writeModule(char * fileName, void * mem, int length)
{
    int fd;
    int i;
    

    char realFileName[80];
    

    strcpy(realFileName, fileName);
    strcat(realFileName, ".aqmod");
    
    
    fd = open(realFileName,O_CREAT|O_WRONLY|O_TRUNC, 0777);

    i = write(fd, mem, length);
    
    close(fd);
    
    return i;
}

	   
MemStream loadModuleStream(char * fileName)
{
    int fd;
    struct stat sb;

    char * p;
    unsigned int len;
    char realFileName[80];
    

    strcpy(realFileName, fileName);
    strcat(realFileName, ".aqua");
    
    
    
    fd = open(realFileName,O_RDONLY, 0);
    if (fd < 0) {
	printf("Couldn't open [%s], source file\n\n", realFileName);
	exit(1);
    }
//    mprintf("Fd: %d\n", (char *)fd);
    
    stat(realFileName, &sb);
    len = sb.st_size;
    
//    mprintf("Size: %d\n", (char *)len);

    p = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);

    return (NewMemStream(p, len));
    
}


