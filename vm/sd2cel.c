
//
// sd2cel.c
//
// <see corresponding .h file>
// 
//
// $Id: sd2cel.c,v 1.18 2002/02/22 22:23:24 dru Exp $
//
//

#include "crc.h"
#include "cel.h"
#include "runtime.h"
#include "memstream.h"
#include "sd2cel.h"



// C is so lame - I have to have my declarations before my definitions

#include "memstream.h"

// The token

struct TokenStruct
{
    MemStream file;
    int token;
    int intValue;
    int stringValue;
    char * binaryValue;
    int reference;
};


// The philosophy of this module is simple
// Read through the stream and setup all objects
// Some of them will be unknown at the time
// of the first pass. So a second pass is done
// to resolve the unknowns.
// Therefore, all of the routines must
// be able to run twice.

typedef struct TokenStruct * Token;

static void assignObjectId ( Proto proto, int32 id, htab * ht);

static void * resolveReference( int32 id, htab * ht );



static void *  readDictionary(Token token, htab * ht);

static void *  readValue(Token token, htab * ht);

static void *  readString(Token token, htab * ht);

static void *  readBinaryData(Token token, htab * ht);

static void *  readArray(Token token, htab * ht);



// Token routines


static Token NewToken(MemStream ms);

static void freeToken(Token tok);

static void getToken(Token tok);

static void readStringSize(Token tok, int s);

static void readBinaryDataSize(Token tok, int s);

static void * getValue(Token tok);

static uint32 getReference(Token tok);

//static char isEndOfFile(Token tok);

static char isStartOfDictionary(Token tok);

static char isEndOfDictionary(Token tok);

static char isStringToken(Token tok);

static char isEndOfStringToken(Token tok);

static char isBinaryDataToken(Token tok);

static char isEndOfBinaryDataToken(Token tok);

static char isStartOfArray(Token tok);

static char isEndOfArray(Token tok);

static char isIntegerToken(Token tok);

static char isNilToken(Token tok);

static char isTrueToken(Token tok);

static char isFalseToken(Token tok);

static char isReferenceToken(Token tok);



static char errMessage[80];
static char errBuff[160];


char *  sd2celError(void)
{
    return errMessage;
}


void sd2setError(char * str)
{
    strcpy(errMessage, str);
    printf(errMessage);
    exit(1);
}


void assignObjectId( Proto proto, int32 id, htab * ht)
{
   haddInt(ht, id, (void *) proto);
}

void * resolveReference( int32 id, htab * ht )
{
   Proto proto;
   Proto p1;
   uint32 i;

   if ( ! hfindInt(ht, id) ) {
     return nil;
   }

   proto = (Proto) hstuff(ht);

   // Determine the type of reference this is
   i = (uint32) proto & PROTO_REF_TYPE;
   p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);

   switch (i) {
     case PROTO_REF_INT:
       assert(0);
       break;

     case PROTO_REF_PROTO:
       
       // If this is now a forward
       while (PROTO_ISFORWARDER(p1)) {
         p1 = PROTO_FORWARDVALUE(p1);
       } 

       if ( p1 != proto ) {
         hstuff(ht) = p1;
       }

       return p1;
       break;

     case PROTO_REF_ATOM:
       return proto;
       break;
   }

   // A line to make the compiler happy and act as a safety check
   return nil;
}


// Convert the SD memory to real objects
void *  sd2cel(void * mem, unsigned int len)
{
    void * returnObject;

    int x;
    MemStream  memStream;
    Token token;
    htab * ht;

    
    
    // ** READING AN EOF ISN'T ALLOWED AT ALL INSIDE, ERROR OUT
    //    file = f;

    // Setup a memory stream to work over the memory
    memStream = NewMemStream((void *)mem, len);

    // Create the reference->object hash table
    ht = celhcreate(8);
    
    //// Read the header verify it (version, CRC flag)
/*
    seekMS([memStream, 0x18);
    len = readMSCInt(memStream);

    [memStream readSize: len fromStream: f];
    */


    // Verify the CRC
    x = celcrc32(mem, len);

    if (x) {
	sd2setError("Bad CRC check");
        return nil;
    }


    //// Need to verify version here



    // Read through the soup once.  PASS 1
    // It will create all of the objects it can
    seekMS(memStream, 0x1C);
    
    token = NewToken(memStream);

    GCOFF();
    
    getToken(token);

    if (isStartOfDictionary(token) == True) {
        returnObject = readDictionary(token, ht);
    } else {
        sd2setError("Bad Simple Data format (expecting a Dictionary)");
        returnObject = nil;            // Indicate an error
    }
    freeToken(token);




    // Read through the soup a second time.  PASS 2
    // This time it will replace unknown objects with
    // their proper values
    seekMS(memStream, 0x1C);
    
    token = NewToken(memStream);
    getToken(token);
    
    if (isStartOfDictionary(token) == True) {
        returnObject = readDictionary(token, ht);
    } else {
        sd2setError("Bad Simple Data format (expecting a Dictionary)");
        returnObject = nil;            // Indicate an error
    }
    freeToken(token);

    
    freeMemStream(memStream);

    GCON();
    
    return returnObject;

}


void *  readDictionary (Token token, htab * ht)
{
    Proto dictionary;
    Proto specialKey;
    Atom   key;
    char * prevKey;
    void * value;
    uint32    datalength;
    uint32    slotlength;
    uint32    slotAction;
    uint32    slotIndex;
    int    i;
    int32  id;



    // Get the reference value from the token
    id         = (int32)getReference(token);
    // See if it resolves to a dictionary
    dictionary = resolveReference(id, ht);


    // Now get information about the dictionary/proto
    // Get the number of data items
    // *NOTE* - since the getValue function will return a cel
    // object when returning an integer, we must de-object it
    getToken(token);
    if (isIntegerToken(token) == False) {
	  sd2setError("<- Expecting integer for data length. Invalid value.");
	  return nil;
    }
    datalength = objectIntegerValue(getValue(token));

    // Get the number of slot items
    getToken(token);
    if (isIntegerToken(token) == False) {
	  sd2setError("<- Expecting integer for slot length. Invalid value.");
	  return nil;
    }
    slotlength = objectIntegerValue(getValue(token));
    
    // if there wasn't an object associated with the reference,
    // it must be the first pass, the time to create
    if ( dictionary == nil) {
      // The numbers returned are the minimum size required to
      // hold the data. However, our system has an implicit guarantee
      // that the proto's have a nil/empty slot at all times (this should
      // be fixed in case one of the routines goes bad) So for
      // now we add a slot for padding. This is a requirement of a
      // open addressing hash data structure

      // Another tricky thing. The created object will get it's slot
      // count increased when the slots are inserted. HOWEVER, the
      // data array count is not increased. We need to have it set
      // to the final value as we fill it.


      // All of this may or may not change in the future

      dictionary = objectNewWithSize((datalength+1), (slotlength+1));

      protoSetDataCount(dictionary, datalength);

      // Now give the reference this id as a value
      assignObjectId(dictionary, id, ht);
    }

    // Loop through here and read the data values
    for (i = 0; i < datalength; i++) {
	
        // Get the value for the key = value
	value = readValue(token, ht);

	// If that value is in error
	if (value == nil) {
	  sd2setError(" <- Expecting a value for values section in proto. Invalid value");
	  return nil;
	} else {
	    // Since we use nil as an indicator of a return we use _nil to indicate a real Nil value
	    if (stringToAtom("_nil") == value) {
		value = nil;
	    }

	    if (objectSetDataAtIndex(dictionary, i, (uint32) value) == nil) {
		sd2setError(" <-error setting data section in dictionary"); 
		return nil;
	    }
	}	    
    }

    prevKey = (char *)0;
    
    specialKey = nil;
    
    for (i = 0; i < slotlength; i++) {
	
	getToken(token);
	
	// If it is a key for a possible Key = Value
	if (isStringToken(token) == True) {
	    key = readString(token, ht);
	} 
	// Otherwise it is a reference
	else
	if (isReferenceToken(token) == True) {
          key = (Atom) resolveReference(getReference(token), ht);
	} else {
	  sprintf(errBuff, "<- Expecting a key for key/value pair. Invalid value (not a string) prevKey=%s slotCount=%ld, i=%d", prevKey, slotlength, i);
	  sd2setError(errBuff);
	  return nil;
	}

	if (key == stringToAtom("_array")) {
	    specialKey = ArrayParent;
	} else
	if (key == stringToAtom("_float")) {
	    specialKey = FloatParent;
	} else
	if (key == stringToAtom("_string")) {
	    specialKey = StringParent;
	}

	prevKey = atomToString(key);

	// Read the two characters
	slotAction = readMSCChar(token->file);
	slotIndex  = readMSCChar(token->file);


	if (objectInsertSlot(dictionary, (uint32) key, slotAction, slotIndex) == nil) {
	  sd2setError(" <-error inserting slot into dictionary"); 
	  return nil;
	}
    }

    // Re-set the parent slot up
    if (specialKey != nil) {
	objectSetSlot(dictionary, ATOMparent, PROTO_ACTION_GET, (uint32) specialKey);
    }
    

    getToken(token);
	
    if (isEndOfDictionary(token) != True) {
	sd2setError(" <- Expected end of dictionary. Invalid token instead.");
	dictionary = nil;
    }
	
    
    return dictionary;
}


void * readValue(Token token, htab * ht)
{
    void * value;


    getToken(token);
    
    if (isStartOfDictionary(token) == True) {
	value = readDictionary(token, ht);
	return value;
    }

    if (isStartOfArray(token) == True) {
	value = readArray(token, ht);
	return value;
    }

    if (isStringToken(token) == True) {
	value = readString(token, ht);
	return value;
    }

    if (isBinaryDataToken(token) == True) {
	value = readBinaryData(token, ht);
	return value;
    }

    if (isIntegerToken(token) == True) {
	value = getValue(token);
	return value;
    }

    if (isNilToken(token) == True) {
	value = stringToAtom("_nil");
	return value;
    }

    if (isTrueToken(token) == True) {
	value = TrueObj;
	return value;
    }

    if (isFalseToken(token) == True) {
	value = FalseObj;
	return value;
    }

    if (isReferenceToken(token) == True) {
	value = resolveReference(getReference(token), ht);
	return value;
    }

    return nil;

}

void * readString(Token token, htab * ht)
{
    void * value;

    // Get a copy of the string
    value = getValue(token);

    // Place string in id->object map
    assignObjectId(value, getReference(token), ht);

    // Get next token to verify that it is an EndOfStringToken
    getToken(token);

    
    if (isEndOfStringToken(token) != True) {
	sd2setError("<- Bad End Of String or Length");
	return nil;
    }

    return value;
}

void * readBinaryData(Token token, htab * ht)
{
    void * value;

    value = getValue(token);

    // Place string in id->object map
    assignObjectId(value, getReference(token), ht);
    
    // Get next token to verify that it is an EndOfBlobToken
    getToken(token);

    if (isEndOfBinaryDataToken(token) != True) {
	sd2setError("Bad Binary Data Section");
	return nil;
    }

    return value;
}


// Note this is just the objectBlob in the array.
// The actual proto that holds the array is in
// readDictionary above.
void * readArray(Token token, htab * ht)
{
    void * value;
    int length;
    int i;
    Proto blob;
    
    
    length = (int)getValue(token);
    blob = objectNewObjectBlob(length);
    
    // Place array in id->object map
    assignObjectId(blob, getReference(token), ht);
    
    for (i = 0; i < length; i++) {
	
	value = (void *)readValue(token, ht);
	
	
	if (value == nil) {
	    if (isEndOfArray(token) == True) {
		sd2setError("Unexpected end of array");
		return nil;
	    } else {
		sd2setError("Unexpected token in array");
		return nil;
	    }
	}
	
	    
	// Since we use nil as an indicator of a return we use _nil to indicate a real Nil value
	if (stringToAtom("_nil") == value) {
	    value = nil;
	}

	objectBlobAtSet(blob, i, value);
    }
    
    getToken(token);
	
    if (isEndOfArray(token) != True) {
	sd2setError(" <- Expected end of array. Invalid token instead.");
	blob = nil;
    }
	
    return blob;
}




#define ENDOFFILE 0
#define STARTOFDICTIONARY 1
#define ENDOFDICTIONARY 2

#define STARTOFARRAY 3
#define ENDOFARRAY 4

#define EQUALTOKEN 5
#define COMMATOKEN 6
#define SEMICOLONTOKEN 7
#define REFERENCETOKEN 8
#define BADTOKEN 9
#define STRINGTOKEN 10
#define ENDOFSTRINGTOKEN 11
#define BINARYDATATOKEN 12
#define ENDOFBINARYDATATOKEN 13
#define INTEGERTOKEN 14
#define NILTOKEN 15
#define TRUETOKEN 16
#define FALSETOKEN 17


// See the SimpleData documentation for the description of the file format

Token NewToken(MemStream ms)
{
    Token tok = celcalloc(sizeof(struct TokenStruct));
    tok->token = BADTOKEN;
    tok->file = ms;
    return tok;
}


void freeToken(Token tok)
{
    celfree(tok);
}


//
// The Start - Read a token in (ignoring whitespace)
//
void getToken(Token tok)
{

    int c;

    while (1) {

	c = readMSCChar(tok->file);

	if (c == EOF) {
	    tok->token = ENDOFFILE;
	    return;
	}
	
	if (c == '{') {
	    tok->token = STARTOFDICTIONARY;
//	    clear(stringValue);
	    tok->reference = readMSCInt(tok->file);
//	    tok->intValue  = readMSCInt(tok->file);
	    return;
	}

	if (c == '}') {
	    tok->token = ENDOFDICTIONARY;
	    return;
	}
	
	if (c & 0x80) {
	    tok->token = STRINGTOKEN;
	    tok->reference = readMSCInt(tok->file);
	    readStringSize(tok, (c & 0x7f));
	    return;
	}
	
	if (c == '~') {
	    tok->token = ENDOFSTRINGTOKEN;
	    return;
	}

	if (c == 'B') {
	    tok->token = BINARYDATATOKEN;
	    tok->reference = readMSCInt(tok->file);
	    tok->intValue = readMSCInt(tok->file);
	    readBinaryDataSize(tok, tok->intValue);
	    return;
	}

	if (c == 'b') {
	    tok->token = ENDOFBINARYDATATOKEN;
	    return;
	}
	
	if (c == '(') {
	    tok->token = STARTOFARRAY;
	    tok->reference = readMSCInt(tok->file);
	    tok->intValue  = readMSCInt(tok->file);
	    return;
	}

	if (c == ')') {
	    tok->token = ENDOFARRAY;
	    return;
	}

	if (c == 'I') {
	    tok->token = INTEGERTOKEN;
	    tok->intValue = readMSCInt(tok->file);
	    return;
	}

	if (c == 'N') {
	    tok->token = NILTOKEN;
	    return;
	}

	if (c == 't') {
	    tok->token = TRUETOKEN;
	    return;
	}

	if (c == 'f') {
	    tok->token = FALSETOKEN;
	    return;
	}

	if (c == 'r') {
	    tok->token = REFERENCETOKEN;
	    tok->reference = readMSCInt(tok->file);
	    return;
	}


      tok->token = BADTOKEN;
      return;
  }

}


void readStringSize(Token tok, int s)
{
    int c, i;
    char * str;
    char * p;

    
    // Create space for string
    str = celmalloc(((s & 0x7f)+1));
    p = str;

    for (i = 0; i < s; i++) {
	c = readMSCChar(tok->file);

	if (c == EOF) {
	    tok->token = ENDOFFILE;
	    celfree(str);
	    return;
	}
	*p = c;
	p++;
    }

    // Terminate the string
    *p = 0;
    
    // Now convert to a string proto
    tok->stringValue = (uint32) objectNewString(str);
    celfree(str);
}


void readBinaryDataSize(Token tok, int s)
{
    char * p;
    Proto  proto;
    
    proto = objectNewBlob(s);
    
    p = objectPointerValue(proto);
    
    readMSSizeInto(tok->file, s, p);
    
    tok->binaryValue = (void *) proto;

}


void *getValue (Token tok)
{
    if (tok->token == STRINGTOKEN) {
	return (void *)tok->stringValue;
    }
    
    if (tok->token == BINARYDATATOKEN) {
	return tok->binaryValue;
    }

    if (tok->token == STARTOFDICTIONARY) {
	return (void *) tok->intValue;
    }

    if (tok->token == STARTOFARRAY) {
	return (void *) tok->intValue;
    }
    
    if (tok->token == INTEGERTOKEN) {
	return (void *) objectNewInteger(tok->intValue);
    }

    return nil;
}

uint32 getReference(Token tok)
{
    return (uint32)tok->reference;
}
    
/*
char isEndOfFile(Token tok)
{
    return (tok->token == ENDOFFILE);
}
*/

char isStartOfDictionary(Token tok)
{
    return (tok->token == STARTOFDICTIONARY);
}

char isEndOfDictionary(Token tok)
{
    return (tok->token == ENDOFDICTIONARY);
}

char isStringToken(Token tok)
{
    return (tok->token == STRINGTOKEN);
}

char isEndOfStringToken(Token tok)
{
    return (tok->token == ENDOFSTRINGTOKEN);
}

char isBinaryDataToken(Token tok)
{
    return (tok->token == BINARYDATATOKEN);
}

char isEndOfBinaryDataToken(Token tok)
{
    return (tok->token == ENDOFBINARYDATATOKEN);
}

char isStartOfArray(Token tok)
{
    return (tok->token == STARTOFARRAY);
}

char isEndOfArray(Token tok)
{
    return (tok->token == ENDOFARRAY);
}

char isIntegerToken(Token tok)
{
    return (tok->token == INTEGERTOKEN);
}

char isNilToken(Token tok)
{
    return (tok->token == NILTOKEN);
}

char isTrueToken(Token tok)
{
    return (tok->token == TRUETOKEN);
}

char isFalseToken(Token tok)
{
    return (tok->token == FALSETOKEN);
}

char isReferenceToken(Token tok)
{
    return (tok->token == REFERENCETOKEN);
}


