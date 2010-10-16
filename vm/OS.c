
//
// OS.c
//
// <see corresponding .h file>
// 
//
// $Id: OS.c,v 1.13 2002/02/22 22:23:24 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "svm32.h"
#include "array.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>

#include "config.h"

#ifdef unix
#include <sys/utsname.h>
#include <sys/wait.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <process.h>
#endif


//
//  OS - this is the wrapper over Unix for Cel
// 

extern char *OSSL[];


extern char **environ;


void OSgetProcessId(void)
{
    Proto p;

    p = objectNewInteger(getpid());
    VMReturn(Cpu, (unsigned int) p, 3);
}

void OSgetGroupId(void)
{
    Proto p;

#ifdef WIN32
    p = objectNewInteger(-1);
#else
    p = objectNewInteger(getgid());
#endif
    VMReturn(Cpu, (unsigned int) p, 3);
}

void OSgetUserId(void)
{
    Proto p;

#ifdef WIN32
    p = objectNewInteger(-1);
#else
    p = objectNewInteger(getuid());
#endif
    VMReturn(Cpu, (unsigned int) p, 3);
}

void OSgetCurrentDir(void)
{
    char buff[4096];
    Proto p;

    if (getcwd(buff, 4096) == NULL) {
	assert(0);
    }
    p = (Proto) stringToAtom(buff);
    
    VMReturn(Cpu, (unsigned int) p, 3);
}

void OSfork(void)
{
    Proto p;

#ifdef WIN32
    p = objectNewInteger(-1);
#else
    p = objectNewInteger(fork());
#endif
    VMReturn(Cpu, (unsigned int) p, 3);
}

#ifdef WIN32
void OSexec(void)
{
    Proto p;
    p = objectNewInteger(-1);
    VMReturn(Cpu, (unsigned int) p, 4);
}
#else
void OSexec(void)
{
    Proto target;
    Proto blob;
    Proto tmpObj;
    char buff[128];
    int size;
    int i;
    char * p;
    char * tp;
    char ** ap;
    char ** argv;
    char * args[2];
    

    target    = (Proto) stackAt(Cpu,4);

    tp = NULL;
    size = 0;
    // Normalize the target
    i = PROTO_REFTYPE(target);
    if (i == PROTO_REF_ATOM) {
	tp = atomToString((Atom)target);
	size = strlen(tp);
    }
    else if (i == PROTO_REF_PROTO) {
	i = objectGetSlot(target, stringToAtom("_string"), &blob);
	if (i == 0) {
	    tp = "";
	    size = 0;
	} else {
	    tp = objectPointerValue(blob);
	    assert(objectGetSlot(target, stringToAtom("size"), &tmpObj));
	    size = objectIntegerValue(tmpObj);
	}
    }

    // Make a copy of the command
    assert(size < 126);
    memcpy(buff, tp, size);
    buff[size] = 0;
    tp = buff;


    // My old hack was to just run a '/bin/sh -c ' and let
    // it parse the command line. Nowadays, I will parse the thing
    p = strchr(tp, ' ');
    if (p == NULL) {
	args[0] = tp;
	args[1] = "";
	execve(tp, args, environ);
    } else {
	argv = celcalloc((sizeof(char *)) * (25)); // No need to free, we are leaving this process
	for (ap = argv; (*ap = strsep(&tp, " \t")) != NULL;) {
	    if (**ap != '\0') {
		if (++ap >= &argv[24]) {
		    break;
		}
	    }
	}
	execve(buff, argv, environ);
    }
	
    // We shouldn't get here

}
#endif

void OSwait(void)
{
    Proto p;
    Proto statObj;
#ifdef WIN32
    p = objectNewInteger(-1);
    statObj = createArray(nil);
    addToArray(statObj, p);
#else
    int status;

    p = objectNewInteger(wait(&status));

    statObj = createArray(nil);
    addToArray(statObj, p);
    
    addToArray(statObj, objectNewInteger(WEXITSTATUS(status)));
#endif
    
    VMReturn(Cpu, (unsigned int) statObj, 3);
}

void OSwaitPid(void)
{
    Proto p;
    Proto statObj;
#ifdef WIN32
    p = objectNewInteger(-1);
    statObj = createArray(nil);
    addToArray(statObj, p);
#else
    Proto arg;
    int status;
    int i;

    arg = (Proto) stackAt(Cpu,4);

    i = waitpid(objectIntegerValue(arg), &status, 0);
    
    p = objectNewInteger(i);

    statObj = createArray(nil);
    addToArray(statObj, p);
    
    addToArray(statObj, objectNewInteger(WEXITSTATUS(status)));
#endif
    
    VMReturn(Cpu, (unsigned int) statObj, 4);
}

void OSsleep(void)
{
    Proto p;
    Proto arg;

    arg    = (Proto) stackAt(Cpu,4);
#ifdef WIN32
    Sleep(objectIntegerValue(arg));
    p = objectNewInteger(0);
#else
    //// Need to implement a millisecond resolution sleep
    p = objectNewInteger(sleep(objectIntegerValue(arg)));
#endif
    VMReturn(Cpu, (unsigned int) p, 4);
}

void OSkillwith(void)
{
    Proto p;

#ifdef WIN32
    p = objectNewInteger(-1);
#else
    Proto pidObj;
    Proto signalObj;
    int   pid;
    int   sig;
    

    pidObj    = (Proto) stackAt(Cpu,4);
    signalObj = (Proto) stackAt(Cpu,5);

    pid = objectIntegerValue(pidObj);
    sig = objectIntegerValue(signalObj);
    
    p = objectNewInteger(kill(pid, sig));
#endif

    VMReturn(Cpu, (unsigned int) p, 5);
}


void OSopenForWrite (void)
{
    Proto p;
    Proto arg;
    int fd;
    char * ap;

    arg    = (Proto) stackAt(Cpu,4);

    ap = string2CString(arg);

    fd = open(ap, O_WRONLY|O_CREAT|O_TRUNC, 0770);

    celfree(ap);
    
    p = objectNewInteger(fd);
    VMReturn(Cpu, (unsigned int) p, 4);
}

void OSopenForRead (void)
{
    Proto p;
    Proto arg;
    int fd;
    char * ap;

    arg    = (Proto) stackAt(Cpu,4);

    ap = string2CString(arg);
    
    fd = open(ap, O_RDONLY);

    celfree(ap);
    
    p = objectNewInteger(fd);
    VMReturn(Cpu, (unsigned int) p, 4);
}

void OSclose(void)
{
    Proto p;
    Proto arg;

    arg    = (Proto) stackAt(Cpu,4);
    p = objectNewInteger(close(objectIntegerValue(arg)));
    VMReturn(Cpu, (unsigned int) p, 4);
}

void OSexit(void)
{
    Proto arg;

    arg    = (Proto) stackAt(Cpu,4);
    exit(objectIntegerValue(arg));
    // As if :-)
    VMReturn(Cpu, (unsigned int) arg, 3);
}

void OSgetUname(void)
{
    Proto p;
#ifdef unix
    struct utsname name;
#endif

    GCOFF();

//library/en-us/sysinfo/sysinfo_92jy.asp

#ifdef WIN32
	p = nil;
#else
    if (uname(&name) == 0) {
	p = objectNew(0);
	objectSetSlot(p, stringToAtom("sysname"), PROTO_ACTION_GET, (uint32) stringToAtom(name.sysname));
	objectSetSlot(p, stringToAtom("nodename"), PROTO_ACTION_GET, (uint32) stringToAtom(name.nodename));
	objectSetSlot(p, stringToAtom("release"), PROTO_ACTION_GET, (uint32) stringToAtom(name.release));
	objectSetSlot(p, stringToAtom("version"), PROTO_ACTION_GET, (uint32) stringToAtom(name.version));
	objectSetSlot(p, stringToAtom("machine"), PROTO_ACTION_GET, (uint32) stringToAtom(name.machine));
    } else {
	p = nil;
    }
#endif
    
    GCON();
    
    VMReturn(Cpu, (unsigned int) p, 3);
}

void OSgetHostname(void)
{
    Atom p;
#ifdef WIN32
    p = nil;
#else
    char buff[64];
    
    if (gethostname(buff, sizeof(buff)) == 0) {
	p = stringToAtom(buff);
    } else {
	p = nil;
    }
    
#endif
    VMReturn(Cpu, (unsigned int) p, 3);
}

#ifdef WIN32
void OScreatePipe(void)
{
    Proto p;
    p = objectNewInteger(-1);
    VMReturn(Cpu, (unsigned int) p, 3);
}
#else
void OScreatePipe(void)
{
    int fds[2];
    Proto array;
    int i;
    
    
    i = pipe(fds);

    if (i != 0) {
	VMReturn(Cpu, (unsigned int) nil, 3);
	return;
    }
    
    array = createArray(nil);

    addToArray(array, objectNewInteger(fds[0]));
    addToArray(array, objectNewInteger(fds[1]));

    VMReturn(Cpu, (unsigned int) array, 3);
}
#endif

void OSdup2(void)
{
    Proto target;
    Proto oldFdObj;
    Proto newFdObj;
    int i;
    int new, old;
    
    
    target      = (Proto) stackAt(Cpu,2);
    oldFdObj    = (Proto) stackAt(Cpu,4);
    newFdObj    = (Proto) stackAt(Cpu,5);

    old = objectIntegerValue(oldFdObj);
    new = objectIntegerValue(newFdObj);

    i = dup2(old, new);
    
    if (i == -1) {
	VMReturn(Cpu, (unsigned int) nil, 5);
	return;
    }
    
    VMReturn(Cpu, (unsigned int) target, 5);
}

void OSunlink(void)
{
    Proto target;
    int i;
    Proto obj;
    char tbuff[128];

    target    = (Proto) stackAt(Cpu,4);

    getString(target, tbuff, sizeof(tbuff));
    
    i = unlink(tbuff);
    
    obj = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) obj, 4);
}

void OSrenameTo(void)
{
    Proto target;
    Proto obj;
    int i;
    Proto arg;
    char  tbuff[128];
    char  abuff[128];

    target    = (Proto) stackAt(Cpu,4);
    arg       = (Proto) stackAt(Cpu,5);

    getString(target, tbuff, sizeof(tbuff));
    getString(arg,    abuff, sizeof(abuff));

    i = rename(tbuff, abuff);

    obj = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) obj, 5);
}



void OSreadFdIntoSize(void)
{
    Proto buffObj;
    Proto sizeObj;
    Proto tmpObj;
    Proto fdObj;
    char * buff = "";
    int fd;
    int size = 0;
    int stringSize = 0;
    Proto self;
    int i;
    

    self    = (Proto) stackAt(Cpu,2);
    fdObj   = (Proto) stackAt(Cpu,4);
    buffObj = (Proto) stackAt(Cpu,5);
    sizeObj = (Proto) stackAt(Cpu,6);

    fd = objectIntegerValue(fdObj);


    if (objectGetSlot(buffObj, stringToAtom("_string"), &tmpObj) ){
	buff = objectPointerValue(tmpObj);
	assert(objectGetSlot(buffObj, stringToAtom("capacity"), &tmpObj) );
	stringSize = objectIntegerValue(tmpObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 6);
    }
    
    if (PROTO_REFTYPE(sizeObj) == PROTO_REF_INT) {
	size = objectIntegerValue(sizeObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 6);
    }

    if (size > stringSize) {
	VMReturn(Cpu, (unsigned int) nil, 6);
    }
    
    i = read(fd, buff, size);

    // Set the new size if no error
    if (i > 0) {
	objectSetSlot(buffObj, stringToAtom("size"), PROTO_ACTION_GET, (uint32) objectNewInteger(i));
    }
    

    VMReturn(Cpu, (unsigned int) objectNewInteger(i), 6);
}


void OSwriteFdFrom (void)
{
    Proto buffObj;
    Proto tmpObj;
    Proto fdObj;
    Proto self;
    char * buff = "";
    int fd;
    int stringSize = 0;
    int i;
    
    self    = (Proto) stackAt(Cpu,2);
    fdObj   = (Proto) stackAt(Cpu,4);
    buffObj = (Proto) stackAt(Cpu,5);

    fd = objectIntegerValue(fdObj);

    if (PROTO_REFTYPE(buffObj) == PROTO_REF_ATOM) {
	buff = objectPointerValue(buffObj);
	stringSize = strlen(buff);
    }
    else
    if (objectGetSlot(buffObj, stringToAtom("_string"), &tmpObj) ){
	buff = objectPointerValue(tmpObj);
	assert(objectGetSlot(buffObj, stringToAtom("size"), &tmpObj) );
	stringSize = objectIntegerValue(tmpObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 5);
    }
    
    i = write(fd, buff, stringSize);

    VMReturn(Cpu, (unsigned int) objectNewInteger(i), 5);
}


#ifdef WIN32
void OSflock(void)
{
    Proto p;
    p = objectNewInteger(-1);
    VMReturn(Cpu, (unsigned int) p, 4);
}
#else
void OSflock (void)
{
    Atom mesg;
    Proto fdObj;
    Proto self;
    int fd;
    int i = 0;
    struct flock lockStruct;
    
    
    self    = (Proto) stackAt(Cpu,2);
    mesg    = (Atom) stackAt(Cpu,3);
    fdObj   = (Proto) stackAt(Cpu,4);

    fd = objectIntegerValue(fdObj);

    lockStruct.l_whence = 0;
    lockStruct.l_start  = 0;
    lockStruct.l_len    = 0;
    lockStruct.l_pid    = 0;

    if (mesg == stringToAtom("exclusiveLockFd:")) {
	lockStruct.l_type = F_WRLCK;
	i = fcntl(fd, F_SETLKW, &lockStruct);
    }
    else if (mesg == stringToAtom("unLockFd:")) {
	lockStruct.l_type = F_UNLCK;
	i = fcntl(fd, F_SETLKW, &lockStruct);
    }

    VMReturn(Cpu, (unsigned int) objectNewInteger(i), 4);
}
#endif


void OSsystem(void)
{
    Proto target;
    Proto obj;
    int i;
    char tbuff[128];
    
    target    = (Proto) stackAt(Cpu,4);

    getString(target, tbuff, sizeof(tbuff));
    
    i = system(tbuff);
    
    obj = objectNewInteger(i);
    VMReturn(Cpu, (unsigned int) obj, 4);
}

void OSlistDirectory(void)
{
    Proto target;
    Proto obj;
    Proto newObj;
    char tbuff[512];
    DIR * dirp;
    struct dirent * dp;
    
    
    target    = (Proto) stackAt(Cpu,4);

    GCOFF();
    
    obj    = createArray(nil);


    getString(target, tbuff, sizeof(tbuff));

    dirp = opendir(tbuff);
    
    while ((dp = readdir(dirp)) != NULL) {
	// Skip '.'
	if ( !strcmp(dp->d_name, ".") )
	    {
		continue;
	    }
		
	// Skip '..'
	if ( !strcmp(dp->d_name, "..") )
	    {
		continue;
	    }
		
	newObj = createString(dp->d_name, strlen(dp->d_name));
	
	addToArray(obj, newObj);
    }
    
    closedir(dirp);
    
    GCON();

    VMReturn(Cpu, (unsigned int) obj, 4);
}


char *OSSL[] = {
	"getProcessId", "OSgetProcessId", (char *) OSgetProcessId,
	"getGroupId", "OSgetGroupId", (char *) OSgetGroupId,
	"getUserId", "OSgetUserId", (char *) OSgetUserId,
	"getCurrentDir", "OSgetCurrentDir", (char *) OSgetCurrentDir,
	"fork",      "OSfork", (char *) OSfork,
	"exec:",      "OSexec", (char *) OSexec,
	"wait",      "OSwait", (char *) OSwait,
	"waitPid:", "OSwaitPid", (char *) OSwaitPid,
	"sleep:", "OSsleep", (char *) OSsleep,
	"kill:with:", "OSkillwith", (char *) OSkillwith,
	"openForWrite:", "OSopenForWrite", (char *) OSopenForWrite,
	"openForRead:", "OSopenForRead", (char *) OSopenForRead,
	"close:", "OSclose", (char *) OSclose,
	"readFd:Into:Size:", "OSreadFdIntoSize", (char *) OSreadFdIntoSize,
	"writeFd:From:", "OSwriteFdFrom", (char *) OSwriteFdFrom,
// Notice use of same method
	"exclusiveLockFd:", "OSflock", (char *) OSflock,
	"unLockFd:", "OSflock", (char *) OSflock,
	"exit:", "OSexit", (char *) OSexit,
	"getUname", "OSgetUname", (char *) OSgetUname,
	"getHostname", "OSgetHostname", (char *) OSgetHostname,
	"createPipe", "OScreatePipe", (char *) OScreatePipe,
	"dupFd:to:", "OSdup2", (char *) OSdup2,
	"unlink:", "OSunlink", (char *) OSunlink,
	"rename:To:", "OSrenameTo", (char *) OSrenameTo,
	"system:", "OSsystem", (char *) OSsystem,
	"listDirectory:", "OSlistDirectory", (char *) OSlistDirectory,
	""
};
