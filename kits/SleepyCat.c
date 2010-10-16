
//
// SleepyCat.c
//
// <see corresponding .h file>
// 
//
// $Id: SleepyCat.c,v 1.7 2002/02/22 18:45:44 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "svm32.h"
#include <time.h>
#include "db.h"
#include <errno.h>
#include <string.h>

//
//  SleepyCat - this is the wrapper over the SleepyCat database library
// 

extern char *SleepyCatSL[];

// This creates an environment handle for Sleepy Cat
void SleepyCatnew(void)
{
    Proto  proto;
    Proto  blob;
    Proto  parent;
    char * p;
    DB *   dbp;
    int    ret;
    
    GCOFF();
    
    proto = objectNew(18);

    blob = objectNewBlob( sizeof(DB *) );
    p = objectPointerValue(blob);
    
    if ( (ret = db_create(&dbp, NULL, 0)) != 0) {
       printString(db_strerror(ret));
       assert(0);
    }

    memcpy(p, &dbp, sizeof(DB *));

    objectGetSlot(Globals, stringToAtom("SleepyCat"), &parent);
    objectSetSlot(proto, ATOMparent, PROTO_ACTION_GET, (uint32) parent);

    objectSetSlot(proto, stringToAtom("_dbp"), PROTO_ACTION_GET, (uint32) blob);
    
    GCON();
    
    VMReturn(Cpu, (unsigned int) proto, 3);
}


// This sets the dbhandle to allow duplicate keys
// 
void SleepyCatsetDuplicates(void)
{
    Proto  blob;
    Proto  proto;
    int    ret;
    char * p;
    DB * dbp;

    proto  = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));
    
    if ( (ret = dbp->set_flags( dbp, DB_DUP )) != 0) {
       printString(db_strerror(ret));
       assert(0);
    }

    VMReturn(Cpu, (unsigned int) proto, 3);
}

// This sets any further opens to be read only
// 
void SleepyCatsetReadOnly(void)
{
    Proto  proto;

    proto  = (Proto) stackAt(Cpu, 2);

    objectSetSlot(proto, stringToAtom("_db_ro"), PROTO_ACTION_GET, (uint32) TrueObj);

    VMReturn(Cpu, (unsigned int) proto, 3);
}

// This takes the _dbp blob and opens an existing database
// 
void SleepyCatopenDbfromFile(void)
{
    Proto  blob;
    Proto  dbName;
    Proto  fileName;
    Proto  proto;
    Proto  tmp;
    char   file[128];
    char   db[128];
    int    ret;
    int    flags;
    char * p;
    DB * dbp;

    proto  = (Proto) stackAt(Cpu, 2);
    dbName = (Proto) stackAt(Cpu, 4);
    fileName = (Proto) stackAt(Cpu, 5);

    getString(dbName, db, sizeof(db));
    getString(fileName, file, sizeof(file));

    objectGetSlot(proto, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));

    flags = 0;
    if (objectGetSlot(proto, stringToAtom("_db_ro"), &tmp)) {
	flags = flags | DB_RDONLY;
    }

    
    if ( (ret = dbp->open( dbp, file, db, DB_UNKNOWN, flags, 0644)) != 0) {
       printString(db_strerror(ret));
       assert(0);
    }

    VMReturn(Cpu, (unsigned int) proto, 5);
}


void SleepyCatcreateDbinFileofType(void)
{
    Proto dbName;
    Proto fileName;
    Proto dbType;
    Proto proto;
    Proto blob;
    char * p;
    DB *  dbp;
    char  db[64];
    char  file[64];
    int   type;
    int   ret;
    
    proto  = (Proto) stackAt(Cpu, 2);
    dbName   = (Proto) stackAt(Cpu, 4);
    fileName = (Proto) stackAt(Cpu, 5);
    dbType   = (Proto) stackAt(Cpu, 6);

    getString(dbName, db, sizeof(db));
    getString(fileName, file, sizeof(file));
    type = objectIntegerValue(dbType);
    
    objectGetSlot(proto, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));
    
    if ( (ret = dbp->open( dbp, file, db, type, DB_CREATE, 0644)) != 0) {
        printString(db_strerror(ret));
        printString("\n");
	assert(0);
    }
    
    VMReturn(Cpu, (unsigned int) proto, 6);
}

// This takes the _dbp blob and removes an existing database
// 
void SleepyCatremoveDbfromFile(void)
{
    Proto  blob;
    Proto  dbName;
    Proto  fileName;
    Proto  proto;
    Proto  result;
    char   file[128];
    char   db[128];
    int    ret;
    char * p;
    DB * dbp;

    proto  = (Proto) stackAt(Cpu, 2);
    dbName = (Proto) stackAt(Cpu, 4);
    fileName = (Proto) stackAt(Cpu, 5);

    getString(dbName, db, sizeof(db));
    getString(fileName, file, sizeof(file));

    objectGetSlot(proto, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));
    
    result = TrueObj;
    
    if ( (ret = dbp->remove( dbp, file, db, 0 )) != 0) {
	if (ret != ENOENT) {
	    printString(db_strerror(ret));
	    assert(0);
	}
	result = FalseObj;
    }

    VMReturn(Cpu, (unsigned int) result, 5);
}

void SleepyCatget (void)
{
    char * keyData;
    int    keySize;
    DBT key, data;
    Proto keyObj;
    Proto newObj;
    Proto proto;
    Proto blob;
    char * p;
    DB *  dbp;
    int   ret;
    
    
    proto    = (Proto) stackAt(Cpu, 2);
    keyObj   = (Proto) stackAt(Cpu, 4);

    getStringPtr(keyObj, &keyData, &keySize);
    
    objectGetSlot(proto, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));

    
    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    
    key.data = keyData;
    key.size = keySize;
    
    ret = dbp->get(dbp, NULL, &key, &data, 0);
    if (ret == 0) {
	newObj = createString(data.data, data.size);
    }
    else if (ret == DB_NOTFOUND) {
	newObj = FalseObj;
    } else {
	assert(0);
    }

    VMReturn(Cpu, (unsigned int) newObj, 4);
}

void SleepyCatsetWith (void)
{
    char * keyData;
    int    keySize;
    char * dataData;
    int    dataSize;
    DBT key, data;
    Proto keyObj;
    Proto proto;
    Proto dataObj;
    Proto blob;
    char * p;
    DB *  dbp;
    int ret;
    
    
    proto     = (Proto) stackAt(Cpu, 2);
    keyObj    = (Proto) stackAt(Cpu, 4);
    dataObj   = (Proto) stackAt(Cpu, 5);

    getStringPtr(keyObj, &keyData, &keySize);
    getStringPtr(dataObj, &dataData, &dataSize);
    
    objectGetSlot(proto, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));
    
    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    
    key.data = keyData;
    key.size = keySize;
    
    data.data = dataData;
    data.size = dataSize;
    
    if ( (ret = dbp->put( dbp, NULL, &key, &data, 0)) != 0) {
        printString(db_strerror(ret));
        printString("\n");
	assert(0);
    }

    VMReturn(Cpu, (unsigned int) proto, 5);
}


void SleepyCatrecordCount (void)
{
    Proto  proto;
    Proto  blob;
    char * p;
    DB   * dbp;
    int    count;
    int    ret;
    void * sp;
    DB_HASH_STAT * hsp;
    
    
    
    proto    = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));
 
   
    if ( (ret = dbp->stat(dbp, &sp, 0)) != 0 ) {
	printString(db_strerror(ret));
	assert(0);
    }

    hsp = (DB_HASH_STAT *) sp;

    count = hsp->hash_ndata;

    free(sp);
    
    proto = objectNewInteger(count);

    VMReturn(Cpu, (unsigned int) proto, 3);
}

void SleepyCatclose(void)
{
    Proto proto;
    Proto blob;
    char * p;
    DB *  dbp;
    
    proto    = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));
    
    if (dbp->close( dbp, 0) != 0) {
	assert(0);
    }

    VMReturn(Cpu, (unsigned int) proto, 3);
}

void SleepyCatcursorClose(void)
{
    Proto proto;
    Proto blob;
    char * p;
    DBC *  dbc;
    
    proto    = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_dbc"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbc, p, sizeof(DBC *));
    
    if (dbc->c_close(dbc) != 0) {
	assert(0);
    }

    VMReturn(Cpu, (unsigned int) proto, 3);
}


// This creates a cursor from an open database
void SleepyCatcreateCursor(void)
{
    Proto  proto;
    Proto  blob;
    Proto  parent;
    Proto  dbObj;
    char * p;
    DB *   dbp;
    int    ret;
    DBC * dbc;
    
    
    dbObj    = (Proto) stackAt(Cpu, 2);

    objectGetSlot(dbObj, stringToAtom("_dbp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbp, p, sizeof(DB *));
    
    if ( (ret = dbp->cursor(dbp, NULL, &dbc, 0)) != 0 ) {
	printString(db_strerror(ret));
	assert(0);
    }

    GCOFF();
    
    proto = objectNew(14);

    blob = objectNewBlob( sizeof(DBC *) );
    p = objectPointerValue(blob);
    
    memcpy(p, &dbc, sizeof(DBC *));

    objectGetSlot(Globals, stringToAtom("SleepyCat"), &parent);
    objectSetSlot(proto, ATOMparent, PROTO_ACTION_GET, (uint32) parent);
    // We need to do this since the 'parent' already has a close for the database handle
    // we need a specific one just for the cursor
    objectSetSlot(proto, stringToAtom("close"), PROTO_ACTION_INTRINSIC, (uint32) SleepyCatcursorClose);

    objectSetSlot(proto, stringToAtom("_dbc"), PROTO_ACTION_GET, (uint32) blob);

    // Preset the key and value slots
    objectEasySetSlot(proto, stringToAtom("key"), (uint32) nil);
    objectEasySetSlot(proto, stringToAtom("value"), (uint32) nil);
    objectEasySetSlot(proto, stringToAtom("_first"), (uint32) TrueObj);
    objectEasySetSlot(proto, stringToAtom("_flags"), (uint32) objectNewInteger(DB_SET));
    
    GCON();
    
    VMReturn(Cpu, (unsigned int) proto, 3);
}

// Set the 'flags' for the cursor such that a 'get' or 'getNext'
// use the DB_SET_RANGE if a key is set.
// Sleepycat will then return the smallest key that is greater than
// or equal to the specified key. return true if things are
// good, return false if the last record is hit or things go wrong
void SleepyCatCursorsetRange (void)
{
    Proto proto;
    
    proto    = (Proto) stackAt(Cpu, 2);

    objectEasySetSlot(proto, stringToAtom("_flags"), (uint32) objectNewInteger(DB_SET_RANGE));

    VMReturn(Cpu, (unsigned int) proto, 3);
}


// Get the value for the cursor, assuming there is a key set in
// the cursor. return true if things are
// good, return false if the last record is hit or things go wrong
// THIS METHOD Also handles 'getRange'
// Get the value for the cursor, assuming there is a key set in
// the cursor, INWHICH the smallest key that is greater than
// or equal to the specified key. return true if things are
// good, return false if the last record is hit or things go wrong
void SleepyCatCursorGet (void)
{
    DBT key, data;
    Proto keyObj;
    Proto dataObj;
    Proto proto;
    Proto retObj;
    Proto flagObj;
    Proto blob;
    char * p;
    DBC *  dbc;
    int ret;
    char * keyData;
    int    keySize;
    int    flags;
    
    
    proto    = (Proto) stackAt(Cpu, 2);


    objectGetSlot(proto, stringToAtom("key"),  &keyObj);
    getStringPtr(keyObj, &keyData, &keySize);

    objectGetSlot(proto, stringToAtom("_flags"),  &flagObj);
    flags = objectIntegerValue(flagObj);
    
    objectGetSlot(proto, stringToAtom("_dbc"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbc, p, sizeof(DBC *));

    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    
    key.data = keyData;
    key.size = keySize;

    ret = dbc->c_get(dbc, &key, &data, flags);

    if (ret == 0) {
	retObj = TrueObj;

	dataObj = createString(data.data, data.size);
	objectSetSlot(proto, stringToAtom("value"), PROTO_ACTION_GET, (uint32) dataObj);
	// Just in case a 'getNext' is used afterwards, it won't use a DB_SET
	// it will use DB_NEXT/PREV
	objectSetSlot(proto, stringToAtom("_first"), PROTO_ACTION_GET, (uint32) FalseObj);
    }
    else if (ret != DB_NOTFOUND) {
	retObj = FalseObj;
    } else {
	retObj = FalseObj;
    }
    
    VMReturn(Cpu, (unsigned int) retObj, 3);
}



// Get the next item for the cursor, return true if things are
// good, return false if the last record is hit or things go wrong
void SleepyCatCursorGetNext (void)
{
    DBT key, data;
    Proto keyObj;
    Proto dataObj;
    Proto proto;
    Proto retObj;
    Proto blob;
    char * p;
    DBC *  dbc;
    Atom  mesg;
    int flags;
    int ret;
    Proto  firstObj;
    Proto  flagObj;
    char * keyData;
    int    keySize;
    
    proto    = (Proto) stackAt(Cpu, 2);
    mesg     = (Atom) stackAt(Cpu, 3);

    if (mesg == stringToAtom("getNext")) {
	flags = DB_NEXT;
    } else {
	flags = DB_PREV;
    }

    // This handles the first call to getNext
    // it handles the situation where a key
    // was initialized
    keyObj = nil;
    objectGetSlot(proto, stringToAtom("_first"),  &firstObj);
    if (firstObj == TrueObj) {
      objectSetSlot(proto, stringToAtom("_first"), PROTO_ACTION_GET, (uint32) FalseObj);
      objectGetSlot(proto, stringToAtom("key"),  &keyObj);
      if (keyObj != nil) {
        getStringPtr(keyObj, &keyData, &keySize);
	objectGetSlot(proto, stringToAtom("_flags"),  &flagObj);
	flags = objectIntegerValue(flagObj);
      }
    }
    
    objectGetSlot(proto, stringToAtom("_dbc"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbc, p, sizeof(DBC *));

    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    
    if (keyObj != nil) {
      key.data = keyData;
      key.size = keySize;
    }

    ret = dbc->c_get(dbc, &key, &data, flags);

    if (ret == 0) {
	retObj = TrueObj;

	GCOFF();
	
	keyObj = createString(key.data, key.size);
	dataObj = createString(data.data, data.size);

	GCON();
	
	objectSetSlot(proto, stringToAtom("key"), PROTO_ACTION_GET, (uint32) keyObj);
	objectSetSlot(proto, stringToAtom("value"), PROTO_ACTION_GET, (uint32) dataObj);
    }
    else if (ret != DB_NOTFOUND) {
	retObj = FalseObj;
    } else {
	retObj = FalseObj;
    }
    
    VMReturn(Cpu, (unsigned int) retObj, 3);
}

// Get the next item for the cursor, that is a duplicate key, return true if things are
// good, return false if the last record is hit or things go wrong
void SleepyCatCursorGetNextDuplicate (void)
{
    DBT key, data;
    Proto dataObj;
    Proto proto;
    Proto retObj;
    Proto blob;
    char * p;
    DBC *  dbc;
    int ret;
    
    proto    = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_dbc"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbc, p, sizeof(DBC *));

    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    
    ret = dbc->c_get(dbc, &key, &data, DB_NEXT_DUP);

    if (ret == 0) {
	retObj = TrueObj;

	dataObj = createString(data.data, data.size);
	objectSetSlot(proto, stringToAtom("value"), PROTO_ACTION_GET, (uint32) dataObj);
    }
    else if (ret != DB_NOTFOUND) {
	retObj = FalseObj;
    } else {
	retObj = FalseObj;
    }
    
    VMReturn(Cpu, (unsigned int) retObj, 3);
}


void SleepyCatgetKeyValue (void)
{
    char * keyData;
    int    keySize;
    char * dataData;
    int    dataSize;
    DBT key, data;
    Proto resultObj;
    Proto keyObj;
    Proto dataObj;
    Proto proto;
    Proto blob;
    char * p;
    DBC *  dbc;
    int   ret;
    
    
    proto    = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("key"),  &keyObj);
    objectGetSlot(proto, stringToAtom("value"), &dataObj);
    
    getStringPtr(keyObj, &keyData, &keySize);
    getStringPtr(dataObj, &dataData, &dataSize);
    
    objectGetSlot(proto, stringToAtom("_dbc"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbc, p, sizeof(DBC *));


    memset(&key,  0, sizeof(key));
    memset(&data, 0, sizeof(data));
    
    key.data = keyData;
    key.size = keySize;
    
    data.data = dataData;
    data.size = dataSize;
    
    ret = dbc->c_get(dbc, &key, &data, DB_GET_BOTH);
    if (ret == 0) {
	resultObj = TrueObj;
    }
    else if (ret == DB_NOTFOUND) {
	resultObj = FalseObj;
    } else {
	assert(0);
    }

    VMReturn(Cpu, (unsigned int) resultObj, 3);
}


void SleepyCatdelete (void)
{
    Proto resultObj;
    Proto proto;
    Proto blob;
    char * p;
    DBC *  dbc;
    int   ret;
    
    
    proto    = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_dbc"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&dbc, p, sizeof(DBC *));

    ret = dbc->c_del(dbc, 0);
    if (ret == 0) {
	resultObj = TrueObj;
    }
    else {
	resultObj = FalseObj;
    }

    VMReturn(Cpu, (unsigned int) resultObj, 3);
}

char *SleepyCatSL[] = {
	"new",    "SleepyCatnew", (char *) SleepyCatnew,
	"setDuplicates", "SleepyCatsetDuplicates", (char *) SleepyCatsetDuplicates,
	"setReadOnly", "SleepyCatsetReadOnly", (char *) SleepyCatsetReadOnly,
	"openDb:fromFile:", "SleepyCatopenDbfromFile", (char *) SleepyCatopenDbfromFile,
	"createDb:inFile:ofType:", "SleepyCatcreateDbinFileofType", (char *) SleepyCatcreateDbinFileofType,
	"removeDb:fromFile:", "SleepyCatremoveDbfromFile", (char *) SleepyCatremoveDbfromFile,
	"get:", "SleepyCatget", (char *) SleepyCatget,
	"set:With:", "SleepyCatsetWith", (char *) SleepyCatsetWith,
	"recordCount", "SleepyCatrecordCount", (char *) SleepyCatrecordCount,
	"close", "SleepyCatclose", (char *) SleepyCatclose,

	// CURSOR METHODS
	"createCursor", "SleepyCatcreateCursor", (char *) SleepyCatcreateCursor,
	"setRange", "SleepyCatCursorsetRange", (char *) SleepyCatCursorsetRange,
	"get", "SleepyCatCursorGet", (char *) SleepyCatCursorGet,
	"getNext", "SleepyCatCursorGetNext", (char *) SleepyCatCursorGetNext,
	"getNextDuplicate", "SleepyCatCursorGetNextDuplicate", (char *) SleepyCatCursorGetNextDuplicate,
	// *NOTE* same function, it handles both methods
	"getPrevious", "SleepyCatCursorGetNext", (char *) SleepyCatCursorGetNext,
	"getKeyValue", "SleepyCatgetKeyValue", (char *) SleepyCatgetKeyValue,
	"delete", "SleepyCatdelete", (char *) SleepyCatdelete,
	""
};

