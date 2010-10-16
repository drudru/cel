
//
// Socket.c
//
// <see corresponding .h file>
// 
//
// $Id: Socket.c,v 1.7 2002/02/22 22:23:24 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "svm32.h"
#include <unistd.h>
#include <string.h>

#include "config.h"

#ifdef unix
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#undef int32;
#include <winsock2.h>
#endif


//
//  Socket - this is the wrapper over Unix for Cel
// 

extern char *SocketSL[];

void Socketinit(void)
{

#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
 
	wVersionRequested = MAKEWORD( 2, 2 );
 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		VMReturn(Cpu, (unsigned int) FalseObj, 3);
		return;
	}

#endif
    VMReturn(Cpu, (unsigned int) TrueObj, 3);
}

void SocketatExit(void)
{

#ifdef WIN32
	int err;
 
	err = WSACleanup();
	//printf("WSACleanup error = %d\n", err);
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		printf("WSACleanup error\n");
		VMReturn(Cpu, (unsigned int) FalseObj, 3);
		return;
	}
#endif
    VMReturn(Cpu, (unsigned int) TrueObj, 3);
}


void SocketconnectToHostPort(void)
{
    Proto hostObj;
    Proto portObj;
    Proto tmpObj;
    Proto self;
    struct hostent * hostInfo;
    int port;
    char * name;
    int sockFd;
    struct sockaddr_in client;
    int i;
    char buff[80];
    

    self    = (Proto) stackAt(Cpu,2);
    hostObj = (Proto) stackAt(Cpu,4);
    portObj = (Proto) stackAt(Cpu,5);

    // Validate the types
    if (PROTO_REFTYPE(hostObj) == PROTO_REF_ATOM) {
	name = objectPointerValue(hostObj);
	strcpy(buff, name);
    } else if (objectGetSlot(hostObj, stringToAtom("_string"), &tmpObj) ){
	name = objectPointerValue(tmpObj);
	assert(objectGetSlot(hostObj, stringToAtom("size"), &tmpObj));
	i = objectIntegerValue(tmpObj);
	memcpy(buff, name, i);
	// Terminate the string
	buff[i] = 0;
    } else {
	VMReturn(Cpu, (unsigned int) nil, 5);
	return;
    }
    name = buff;
    

    if (PROTO_REFTYPE(portObj) == PROTO_REF_INT) {
	port = objectIntegerValue(portObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 5);
	return;
    }

    hostInfo = gethostbyname(name);
    if (NULL == hostInfo) {
#ifdef WIN32
        printf("Error = %d\n", WSAGetLastError());
#endif
	VMReturn(Cpu, (unsigned int) nil, 5);
	return;
    }

    memcpy((void *)&client.sin_addr.s_addr, hostInfo->h_addr_list[0], 4);

    client.sin_family               = AF_INET;
    //??client.sin_addr.s_addr      = htonl(INADDR_ANY);
    client.sin_port                 = htons(port);

    sockFd = socket(PF_INET, SOCK_STREAM, 0);
    objectSetSlot(self, stringToAtom("sockFd"), PROTO_ACTION_GET, (uint32) objectNewInteger(sockFd));

    i = connect(sockFd, (struct sockaddr *) &client, sizeof(client));
    if (i != 0) {
	VMReturn(Cpu, (unsigned int) nil, 5);
	return;
    }
    
    // Return self
    VMReturn(Cpu, (unsigned int) self, 5);
}

void Socketclose(void)
{
    Proto self;
    int sockFd = 0;
    Proto tmpObj;
    int i;
    

    self    = (Proto) stackAt(Cpu,2);

    if (objectGetSlot(self, stringToAtom("sockFd"), &tmpObj) ){
	sockFd = objectIntegerValue(tmpObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 3);
    }
    
#ifdef WIN32
    i = closesocket(sockFd);
#else
    i = close(sockFd);
#endif
    
    if (i == 0) {
	VMReturn(Cpu, (unsigned int) self, 3);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 3);
    }
}

void SocketreadIntoSize(void)
{
    Proto buffObj;
    Proto sizeObj;
    Proto tmpObj;
    char * buff = "";
    int sockFd = 0;
    int size = 0;
    int stringSize = 0;
    Proto self;
    int i;
    

    self    = (Proto) stackAt(Cpu,2);
    buffObj = (Proto) stackAt(Cpu,4);
    sizeObj = (Proto) stackAt(Cpu,5);

    // Validate the params
    if (objectGetSlot(self, stringToAtom("sockFd"), &tmpObj) ){
	sockFd = objectIntegerValue(tmpObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 5);
    }

    if (objectGetSlot(buffObj, stringToAtom("_string"), &tmpObj) ){
	buff = objectPointerValue(tmpObj);
	assert(objectGetSlot(buffObj, stringToAtom("capacity"), &tmpObj) );
	stringSize = objectIntegerValue(tmpObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 5);
    }
    
    if (PROTO_REFTYPE(sizeObj) == PROTO_REF_INT) {
	size = objectIntegerValue(sizeObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 5);
    }

    if (size > stringSize) {
	VMReturn(Cpu, (unsigned int) nil, 5);
    }
    
#ifdef WIN32
    i = recv(sockFd, buff, size, 0);
#else
    i = read(sockFd, buff, size);
#endif 

    // Set the new size if no error
    if (i > 0) {
	objectSetSlot(buffObj, stringToAtom("size"), PROTO_ACTION_GET, (uint32) objectNewInteger(i));
    }
    

    VMReturn(Cpu, (unsigned int) objectNewInteger(i), 5);
}

void SocketwriteFrom (void)
{
    Proto buffObj;
    Proto tmpObj;
    Proto self;
    char * buff = "";
    int sockFd = 0;
    int stringSize = 0;
    int i;
    

    self    = (Proto) stackAt(Cpu,2);
    buffObj = (Proto) stackAt(Cpu,4);

    // Validate the params
    if (objectGetSlot(self, stringToAtom("sockFd"), &tmpObj) ){
	sockFd = objectIntegerValue(tmpObj);
    } else {
	VMReturn(Cpu, (unsigned int) nil, 4);
    }

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
	VMReturn(Cpu, (unsigned int) nil, 4);
    }
    
#ifdef WIN32
    i = send(sockFd, buff, stringSize, 0);
#else
    i = write(sockFd, buff, stringSize);
#endif

    VMReturn(Cpu, (unsigned int) objectNewInteger(i), 4);
}


char *SocketSL[] = {
	"init",                "Socketinit", (char *) Socketinit,
	"atExit",              "SocketatExit", (char *) SocketatExit,
	"connectToHost:port:", "SocketconnectToHostPort", (char *) SocketconnectToHostPort,
	"close",               "Socketclose", (char *) Socketclose,
	"readInto:Size:",      "SocketreadIntoSize", (char *) SocketreadIntoSize,
	"writeFrom:",          "SocketwriteFrom", (char *) SocketwriteFrom,
	""
};
