
//
// compiler.c
//
// <see corresponding .h file>
// 
//
// $Id: compiler.c,v 1.41 2002/01/28 23:15:22 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "memstream.h"
#include "compiler.h"
#include "array.h"
#include "codegen.h"
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

// Globals

char errMessage[80];
char stringBuffer[256];

MemStream  mStream;

static int  consumed  = 1;
static int  ignoreComments = 1;

static int  errorHappened = 0;

// Functional Prototypes for the Parser routines

int readObject(CompilerToken token, Proto parseTree);

int readSlotList(CompilerToken token, Proto parseTree);

int readCodeBlock(CompilerToken token, Proto parseTree);

int readSlot(CompilerToken token, Proto parseTree);

int readUnarySlot(CompilerToken token, Proto parseTree);

int readBinarySlot(CompilerToken token, Proto parseTree);

int readKeywordSlot(CompilerToken token, Proto parseTree);

int readExpression(CompilerToken token, Proto parseTree);

int readMessage(CompilerToken token, Proto parseTree);

int readBinary(CompilerToken token, Proto parseTree);

int readUnary(CompilerToken token, Proto parseTree);

int readUnary(CompilerToken token, Proto parseTree);

int readArray(CompilerToken token, Proto parseTree);

int readReceiver(CompilerToken token, Proto parseTree);

// Parsing routines - parse is the only routine that should be called

Proto  parse(CompilerToken token);


// Functional Prototypes for the CompilerToken routines


CompilerToken NewToken(MemStream ms);

void freeToken(CompilerToken tok);

void getToken(CompilerToken tok);

void realGetToken(CompilerToken tok);

void consumeToken(CompilerToken tok);

void printToken(CompilerToken tok);

void readComment(CompilerToken tok);

void readComment2(CompilerToken tok);

void readString(CompilerToken tok);

int readIdentifierKeyword(CompilerToken tok);

int readArgument(CompilerToken tok);

int readOperator(CompilerToken tok);

int readFloat(CompilerToken tok);

int readFixedPoint(CompilerToken tok);

int readInteger(CompilerToken tok);


void * getValue(CompilerToken tok);

int  getLineNumber(void);

char isBadToken(CompilerToken tok);

char isEndOfFile(CompilerToken tok);

char isCommentToken(CompilerToken tok);

char isStringToken(CompilerToken tok);

char isIdentifierToken(CompilerToken tok);

char isKeywordToken(CompilerToken tok);

char isArgumentToken(CompilerToken tok);

char isOperatorToken(CompilerToken tok);

char isPipeToken(CompilerToken tok);
/*
char isReturnToken(CompilerToken tok);
*/

char isFloatToken(CompilerToken tok);

char isFixedPointToken(CompilerToken tok);

char isIntegerToken(CompilerToken tok);

char isBaseIntegerToken(CompilerToken tok);

char isPeriodToken(CompilerToken tok);

char isLParenToken(CompilerToken tok);

char isRParenToken(CompilerToken tok);

char isLCurlyToken(CompilerToken tok);

char isRCurlyToken(CompilerToken tok);

char isLBracketToken(CompilerToken tok);

char isRBracketToken(CompilerToken tok);



//
// Code
//

char *  compileError(void)
{
    return errMessage;
}



void compileSetError (char * str)
{
    strcpy(errMessage, str);
}

void printError ( char * str, CompilerToken token )
{
    char buff[256];
    
    if (errorHappened) {
	return;
    }
    
    errorHappened = 1;
    
    strcpy(buff, "Error:");
    strcat(buff, str);
    printString(buff);
    printToken(token);
}


// Convert the SD memory to real objects
Proto eval (char * p, int parseShow, int codeGenShow)
{
    MemStream memStream;
    CompilerToken token;
    Proto parseTree;
    Proto syntaxTree;
    Proto module;


    // ** READING AN EOF ISN'T ALLOWED AT ALL INSIDE, ERROR OUT
    //    file = f;

    errorHappened = 0;		// Reinitialize the error flag
    
    memStream = NewMemStream(p, strlen(p));
    
    token = NewToken(memStream);

    GCOFF();
    
    parseTree = parse(token);

    syntaxTree = transform(parseTree);

    if (parseShow) {
      printString("\n\n*** Final Parse Tree ***\n");
      objectDump(syntaxTree);
    }

    module = codeGen(syntaxTree, 0);

    if (codeGenShow) {
      printString("\n\n codeGen Tree \n\n");
      objectDump(module);
    }
    
    freeMemStream(memStream);
    freeToken(token);

    GCON();
    
    return module;

}


// Convert the SD memory to real objects
void *  compile(MemStream memStream, int parseShow, int codeGenShow)
{
    CompilerToken token;
    Proto parseTree;
    Proto syntaxTree;
    Proto module;
    

    
    // ** READING AN EOF ISN'T ALLOWED AT ALL INSIDE, ERROR OUT
    //    file = f;

    errorHappened = 0;		// Reinitialize the error flag
    
    mStream = memStream;
    
    token = NewToken(memStream);

    parseTree = parse(token);

    syntaxTree = transform(parseTree);

    if (parseShow) {
      printString("\n\n*** Final Parse Tree ***\n");
      objectDump(syntaxTree);
    }

    module = codeGen(syntaxTree, 1);

    if (codeGenShow) {
      printString("\n\n codeGen Tree \n\n");
      objectDump(module);
    }
    
    freeToken(token);

    return module;

}

// P A R S I N G - Each function parses a non-terminal. If they return 0, it wasn't found, if they return 1, it was found, if they return -1,
//                 then an error occured while parsing it.

// Build a parse tree
Proto  parse (CompilerToken token)
{
    int i;
    
    Proto parseTree;
    
    parseTree = createArray( (Proto) stringToAtom("Soup"));
    
    // Get object
    i = readObject(token, parseTree);
    
    if (i != 1) {
	printError(" Syntax Error reading soup.\n Something went wrong while reading initial Soup object.\n", token);
	exit(1);
    }
    
    return parseTree;
    
}

// Read object
int readObject(CompilerToken token, Proto parseTree)
{
    Proto privateTree;
    int i;
    
    
    privateTree = createArray( (Proto) stringToAtom("Object"));
    
    getToken(token);
    
    if (isLCurlyToken(token) == True) {
	
	consumeToken(token);
	
	// Get possible slotlist
	i = readSlotList(token, privateTree);
	
	if (i < 0) {
	    printError(" Syntax Error reading slotlist for object.\n", token);
	    return -1;
	}
	
	// Get possible slotlist
	i = readCodeBlock(token, privateTree);
	
	if (i < 0) {
	    printError(" Syntax Error reading code section of object.\n", token);
	    return -1;
	}
	
        getToken(token);
	
	if (isRCurlyToken(token) == True) {
            consumeToken(token);
	    addToArray(parseTree, privateTree);
	    return 1;
	}
	
	if (11111) {
	    
	    printError("Syntax Error reading object.\n Was expecting '}' to terminate object in readObject.\n", token);
	    return -1;
	}
	
    } 
    else {
	return 0;
    }
}


int readSlotList (CompilerToken token, Proto parseTree)
{
    int i;
    int didReadSlot   = 0;
    int lastPeriod = 1;
    
    Proto privateTree;
    
    privateTree = createArray( (Proto) stringToAtom("SlotList"));

    getToken(token);
    
    if (isPipeToken(token) == True) {
	
	// Consume the '|' 
	consumeToken(token);
	
	while (1) {
	    
	    // Get Slot
	    i = readSlot(token, privateTree);
	    
	    if (i < 0) {
		printError(" Syntax Error reading slot.\n", token);
		return -1;
	    }
	    
	    if ((didReadSlot == 0) && (i == 0)) {
		printError(" Syntax Error reading slot.\n Missing a slot definition after a '|'.\n", token);
		return -1;
	    }
	    
	    if (i && !didReadSlot) {
		didReadSlot = 1;
	    }
	    
	    if (i && (lastPeriod == 0)) {
		printError(" Syntax Error reading slot for object.\n Missing a period in between slots.\n", token);
		return -1;
	    }
	    
	    
	    getToken(token);
	    
	    if (isPeriodToken(token) == True) {
		lastPeriod = 1;
	        consumeToken(token);
	    } else {
		lastPeriod = 0;
	    }
	    
	    
	    getToken(token);
	    
	    if (isPipeToken(token) == True) {
	        consumeToken(token);
		break;
	    }

	}
	
	addToArray(parseTree,privateTree);
	return 1;
    }
    else {
	return 0;
    }
}

// Tricky block of code, expressions will always return 1 (since
// they can be nil), therefore, you can't rely on them not
// existing.
int readCodeBlock(CompilerToken token, Proto parseTree)
{
    int i;
    int readReturnToken = 0;
    int lastPeriod      = 1;
    
    Proto privateTree;
    Proto returnArray;
    
    
    privateTree = createArray( (Proto) stringToAtom("CodeBlock"));
    
    while (1) {

	i = readExpression(token, privateTree);

	if (i < 0) {
	    printError(" Syntax Error reading for object.\n Invalid expression in codeblock of object.\n", token);
	    return -1;
	}
	
	// I believe it will always return a 1 since it can be nil

	if (i && (lastPeriod == 0)) {
	    printError(" Syntax Error reading slot for object.\n Missing a period in between expressions.\n", token);
	    return -1;
	}
	    
	    
	getToken(token);
	    
	if (isPeriodToken(token) == True) {
	    lastPeriod = 1;
	    consumeToken(token);
	} else {
	    lastPeriod = 0;
	}
	
	    
	getToken(token);

	if ( isOperatorToken(token) && (getValue(token) == (Proto) stringToAtom("^")) ) {
	    if (lastPeriod == 0) {
		printError(" Syntax Error reading code block for object.\n Missing a period in between expression and return '^'.\n", token);
		return -1;
	    }

	    returnArray = createArray( (Proto) stringToAtom("ReturnToken"));
	    addToArray(returnArray, getValue(token));
	    addToArray(returnArray, (Proto) stringToAtom("Filler to prevent pruning"));
	    addToArray(privateTree, returnArray);
	    consumeToken(token);
	    readReturnToken = 1;
	    continue;
	}

	// If it was just  ' expression '
	if ( (lastPeriod == 0) ) {
	    break;
	}
    }
    
    addToArray(parseTree,privateTree);
    return 1;
}


int readSlot (CompilerToken token, Proto parseTree)
{
    int i;
    
    Proto privateTree;
    Proto tmpValue;

    
    privateTree = createArray( (Proto) stringToAtom("Slot"));

    getToken(token);

    // Arg-Slot
    if (isArgumentToken(token) == True) {
	addToArray(privateTree, (Proto) stringToAtom("Arg-Slot"));
	addToArray(privateTree, (Proto) getValue(token));
	addToArray(parseTree, (Proto) privateTree);
	consumeToken(token);
	return 1;
    }

    // Binary-Slot
    if (isOperatorToken(token) == True) {
	addToArray(privateTree, (Proto) (Proto) stringToAtom("Binary-Slot"));
	i = readBinarySlot(token, privateTree);
	if ( (i < 0) || (i == 0) ) {
	    printError(" Syntax Error reading slot for object.\n The binary operator slot definition is the wrong syntax.\n", token);
	    return -1;
	}
	addToArray(parseTree, privateTree);
	return 1;
    }
	
    // Keyword-Slot
    if (isKeywordToken(token) == True) {
	addToArray(privateTree, (Proto) stringToAtom("Keyword-Slot"));
	i = readKeywordSlot(token, privateTree);
	if ( (i < 0) || (i == 0) ) {
	    printError(" Syntax Error reading slot for object line.\n The keyword slot definition is the wrong syntax.\n", token);
	    return -1;
	}
	addToArray(parseTree, privateTree);
	return 1;
    }

    // Data-Slot
    if (isIdentifierToken(token) == True) {
	tmpValue = getValue(token);
	consumeToken(token);

	getToken(token);

        // Simple Identifier-Slot
	if (isOperatorToken(token) != True) {
	    // This is a plain identifier slot
	    addToArray(privateTree, (Proto) stringToAtom("Identifier-Slot"));
	    addToArray(privateTree, tmpValue);
	    addToArray(parseTree, privateTree);
	    // consumeToken(token);
	    return 1;
        }

        // Simple Data-GetSet-Slot
	if (getValue(token) == (Proto) stringToAtom("<->")) {
	    addToArray(privateTree, (Proto) stringToAtom("Data-SetGet-Slot"));
	    addToArray(privateTree, tmpValue);
	    consumeToken(token);
	    i = readExpression(token, privateTree);
	    if ( (i < 0) || (i == 0) ) {
	        printError(" Syntax Error reading SetGet '<->' slot's expression.\n", token);
	        return -1;
	    }
	    addToArray(parseTree, privateTree);
	    return 1;
	}
	    
        // Simple Data-Get-Slot
	if (getValue(token) == (Proto) stringToAtom("<-")) {
	    addToArray(privateTree, (Proto) stringToAtom("Data-Get-Slot"));
	    addToArray(privateTree, tmpValue);
	    consumeToken(token);
	    i = readExpression(token, privateTree);
	    if ( (i < 0) || (i == 0) ) {
	        printError(" Syntax Error reading the data Get '<-' slot's expression.\n", token);
	        return -1;
	    }
	    addToArray(parseTree, privateTree);
	    return 1;
	}

        // Unary-Slot
	if (getValue(token) == (Proto) stringToAtom("<+")) {
            addToArray(privateTree, (Proto) stringToAtom("Unary-Slot"));
	    addToArray(privateTree, tmpValue);
	    consumeToken(token);
            i = readUnarySlot(token, privateTree);
            if ( (i < 0) || (i == 0) ) {
                printError(" Syntax Error reading slot for object.\n The unary operator slot definition is the wrong syntax.\n", token);
                return -1;
            }
            addToArray(parseTree, privateTree);
            return 1;
        }
	
	    
    }

    return 0;

}

int readUnarySlot(CompilerToken token, Proto parseTree)
{
    int i;
    Proto privateTree;

   
    privateTree = createArray( (Proto) stringToAtom("UnarySlot"));
    addToArray(privateTree, getValue(token));
    
    i = readObject(token, privateTree);
    
    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading slot for object.\n The unary slots method/object slot definition is the wrong syntax.\n", token);
	return -1;
    }
    
    addToArray(parseTree, privateTree);
    return 1;

}

int readBinarySlot(CompilerToken token, Proto parseTree)
{
    int i;
    Proto privateTree;

   
    privateTree = createArray( (Proto) stringToAtom("BinarySlot"));

    // Add the operator
    getToken(token);
    addToArray(privateTree, getValue(token));
    consumeToken(token);
    
    getToken(token);
			     
    // Optional Identifier
    if (isIdentifierToken(token) == True) {
	addToArray(privateTree, getValue(token));
        consumeToken(token);
    }

    getToken(token);

    if (getValue(token) == (Proto) stringToAtom("<+")) {
	addToArray(privateTree, getValue(token));
        consumeToken(token);
    } else {
	printError(" Syntax Error reading Binary slot for object.\n Expecting '<+' for method definition.\n", token);
	    return -1;
    }

    i = readObject(token, privateTree);
    
    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading slot for object.\n The keyword slot definition is the wrong syntax.\n", token);
	return -1;
    }
    
    addToArray(parseTree, privateTree);
    return 1;

}


int readKeywordSlot(CompilerToken token, Proto parseTree)
{
    int i;
    int identifierMode = 0;
    Proto privateTree;

    privateTree = createArray( (Proto) stringToAtom("KeywordSlot"));

    // Get the keyword
    getToken(token);
    addToArray(privateTree, getValue(token));
    consumeToken(token);
    
    getToken(token);

    // if there is an identifier, go into that identifier mode (ie.
    // expect an identifier after each keyword)
    
    // Optional Identifier
    if (isIdentifierToken(token) == True) {
	identifierMode = 1;
	addToArray(privateTree, (Proto) stringToAtom("Identifier"));
	addToArray(privateTree, getValue(token));
        consumeToken(token);
    }
    else if (isKeywordToken(token) == True) {
	addToArray(privateTree, (Proto) stringToAtom("Keyword"));
	addToArray(privateTree, getValue(token));
        consumeToken(token);
    } else {
	printError(" Syntax Error reading Keyword slot for object. Expected an identifier or keyword 1\n", token);
	return -1;
    }
    

    // Read keywords or keyword/identifier pairs
    while(1) {
	getToken(token);
	
	if (getValue(token) == (Proto) stringToAtom("<+")) {
	    addToArray(privateTree, getValue(token));
	    consumeToken(token);
	    break;
	}
	if (isKeywordToken(token)) {
	    addToArray(privateTree, getValue(token));
	    consumeToken(token);

	    getToken(token);
	    
	    if (identifierMode) {
		if (isIdentifierToken(token)) {
		    addToArray(privateTree, (Proto) stringToAtom("Identifier"));
		    addToArray(privateTree, getValue(token));
		    consumeToken(token);
		} else {
		    printError(" Syntax Error reading Keyword slot for object.\n Expected an identifier after the keyword 2\n", token);
		    return -1;
		}
	    }
	    
	} else {
	    printError(" Syntax Error reading Keyword slot for object.\n Expected a '<+' or Keyword Token", token);
	    return -1;
	}
    }

    i = readObject(token, privateTree);
    
    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading slot for object.\n The keyword slot definition is the wrong syntax.\n", token);
	return -1;
    }
    
    addToArray(parseTree, privateTree);
    return 1;

}


int readExpression (CompilerToken token, Proto parseTree)
{
    int i;
    Proto privateTree;

    privateTree = createArray( (Proto) stringToAtom("Expression"));
    
    i = readMessage(token, privateTree);

    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading the expression.\n", token);
	return -1;
    }
    
    addToArray(parseTree, privateTree);
    return 1;
}


int readMessage (CompilerToken token, Proto parseTree)
{
    int i;
    Proto privateTree;

    privateTree = createArray( (Proto) stringToAtom("Message"));
    
    i = readBinary(token, privateTree);

    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading message.\n", token);
	return -1;
    }
    

    getToken(token);

    // Check for a keyword
    if (isKeywordToken(token) == True) {
	//DRUDRU - I don't know why this is here, maybe a leftover from
	// keyword slot recognition
	// addToArray(privateTree, (Proto) stringToAtom("Keyword"));
	addToArray(privateTree, getValue(token));
        consumeToken(token);
    } else {
	// At any rate, we have satisfied this production
	addToArray(parseTree, privateTree);
	return 1;
    }

    i = readBinary(token, privateTree);

    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading message. Invalid expression after the keyword\n", token);
	return -1;
    }
    
    // Read any extra "keyword binary" pairs
    while(1) {

	getToken(token);
	
	if (isKeywordToken(token)) {
	    addToArray(privateTree, getValue(token));
	    consumeToken(token);

	    getToken(token);
	    
            i = readBinary(token, privateTree);

            if ( (i < 0) || (i == 0) ) {
            	printError(" Syntax Error reading message. Invalid expression after the keyword\n", token);
	        return -1;
            }
    
	} else {
	    break;
	}
    }

    addToArray(parseTree, privateTree);
    return 1;
}

int readBinary(CompilerToken token, Proto parseTree)
{
    int i;
    Proto privateTree;

    privateTree = createArray( (Proto) stringToAtom("Binary"));
    
    i = readUnary(token, privateTree);

    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading Binary message.\n", token);
	return -1;
    }
    
    getToken(token);

    // Check for an operator
    if (isOperatorToken(token) == True) {
	// addToArray(privateTree, (Proto) stringToAtom("Operator"));
	addToArray(privateTree, getValue(token));
	consumeToken(token);
    } else {
	// At any rate, we have satisfied this production
	addToArray(parseTree, privateTree);
	return 1;
    }

    i = readUnary(token, privateTree);

    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading binary message. Invalid expression after the binary message\n", token);
	return -1;
    }
    
    // Read any extra "operator unary"  pairs
    while(1) {

	getToken(token);
	
	if (isOperatorToken(token) == True) {
	    addToArray(privateTree, getValue(token));
	    consumeToken(token);

            i = readUnary(token, privateTree);

            if ( (i < 0) || (i == 0) ) {
		printError(" Syntax Error reading binary message. Invalid expression after the binary message\n", token);
		return -1;
            }
    
	} else {
	    break;
	}
    }

    addToArray(parseTree, privateTree);
    return 1;
}


int readUnary(CompilerToken token, Proto parseTree)
{
    int i;
    Proto privateTree;

    privateTree = createArray( (Proto) stringToAtom("Unary"));
    
    i = readReceiver(token, privateTree);

    if ( (i < 0) || (i == 0) ) {
	printError(" Syntax Error reading Receiver of Unary message.\n", token);
	return -1;
    }
    
    getToken(token);

    // Check for an identifier
    if (isIdentifierToken(token) == True) {
//	addToArray(privateTree, (Proto) stringToAtom("UnaryTarget"));
	addToArray(privateTree, getValue(token));
        consumeToken(token);
    } else {
	// At any rate, we have satisfied this production
	addToArray(parseTree, privateTree);
	return 1;
    }

    // Read any extra "identifiers"
    while(1) {
	getToken(token);
	
	if (isIdentifierToken(token) == True) {
	    addToArray(privateTree, getValue(token));
	    consumeToken(token);
	} else {
	    break;
	}
    }
    
    // At any rate, we have satisfied this production
    addToArray(parseTree, privateTree);
    return 1;
}





int readArray(CompilerToken token, Proto parseTree)
{
    Proto privateTree;
    int i;
    
    
    privateTree = createArray( (Proto) stringToAtom("Array"));
    
    getToken(token);
    
    if (isLBracketToken(token) != True) {
	assert(0);
    }
    
    consumeToken(token);

    while (1) {

	getToken(token);
	
	if (isRBracketToken(token) == True) {
	    consumeToken(token);
	    addToArray(parseTree, privateTree);
	    return 1;
	}	    
	
	// Get possible slotlist
	i = readReceiver(token, privateTree);
	
	if (i < 0) {
	    printError(" Syntax Error reading code section of object.\n", token);
	    return -1;
	}
    }

    
    printError("Syntax Error reading array.\n Was expecting ']' to terminate object in readObject.\n", token);
    return -1;

}





int readReceiver(CompilerToken token, Proto parseTree)
{
    int i;
    Proto privateTree;

    privateTree = createArray( (Proto) stringToAtom("Receiver"));
    
    getToken(token);

    // Check for 'self'
    if ( (isIdentifierToken(token) == True) 
         && (getValue(token) == (Proto) stringToAtom("self"))
         ) {
      consumeToken(token);
      // Add the string value 'self'
      addToArray(privateTree, getValue(token));
      addToArray(parseTree, privateTree);
      return 1;
    }

    // Check for a Integer
    if ( (isIntegerToken(token) == True) 
         || (isFixedPointToken(token) == True) ) {
      consumeToken(token);
      addToArray(privateTree, (Proto) stringToAtom("Integer"));
      addToArray(privateTree, getValue(token));
      addToArray(parseTree, privateTree);
      return 1;
    }

    // Check for a Float
    if ( isFloatToken(token) == True ) {
      consumeToken(token);
      addToArray(privateTree, (Proto) stringToAtom("Float"));
      addToArray(privateTree, getValue(token));
      addToArray(parseTree, privateTree);
      return 1;
    }

    // Check for a String
    if ( (isStringToken(token) == True) ) {
      consumeToken(token);
      addToArray(privateTree, (Proto) stringToAtom("String"));
      addToArray(privateTree, getValue(token));
      addToArray(parseTree, privateTree);
      return 1;
    }
    
    // Check for an Array
    if ( (isLBracketToken(token) == True) ) {
	addToArray(privateTree, (Proto) stringToAtom("Array"));
	i = readArray(token, privateTree);
	if ( (i < 0) || (i == 0) ) {
	    printError(" Syntax Error reading receiver. Invalid array in expression.\n", token);
	    return -1;
	}
	addToArray(parseTree, privateTree);
	return 1;
    }

    // Check for an Object
    if ( (isLCurlyToken(token) == True) ) {
	addToArray(privateTree, (Proto) stringToAtom("Object"));
	i = readObject(token, privateTree);
	if ( (i < 0) || (i == 0) ) {
	    printError(" Syntax Error reading receiver. Invalid object/block in expression.\n", token);
	    return -1;
	}
	addToArray(parseTree, privateTree);
	return 1;
    }
    
    // Check for an Expression 
    if ( (isLParenToken(token) == True) ) {
	consumeToken(token);
	addToArray(privateTree, (Proto) stringToAtom("Expression"));
	i = readExpression(token, privateTree);
	if ( (i < 0) || (i == 0) ) {
	    printError(" Syntax Error reading receiver. Invalid parenthesized expression in expression.\n", token);
	    return -1;
	}
	addToArray(parseTree, privateTree);
	getToken(token);
	if ( (isRParenToken(token) == True) ) {
	    consumeToken(token);
	    return 1;
	} else {
	    printError(" Syntax Error reading receiver. Invalid end to parenthesized expression in expression. (ie. where is the ')' to end the expression?\n", token);
	    return -1;
	}
	
    }
    
    
    //
    // nil - an empty expression, therefore the end of the line
    //
    
    addToArray(privateTree, (Proto) stringToAtom("Nil Expression"));
    addToArray(parseTree, privateTree);
    return 1;
}


//
// This is the lexer of the whole deal
//

#define ENDOFFILE             0
#define STARTOFDICTIONARY     1
#define ENDOFDICTIONARY       2

#define STARTOFARRAY          3
#define ENDOFARRAY            4

#define EQUALTOKEN            5
#define COMMATOKEN            6
#define SEMICOLONTOKEN        7
#define BADTOKEN              9
#define STRINGTOKEN          10
#define ENDOFSTRINGTOKEN     11
#define BINARYDATATOKEN      12
#define ENDOFBINARYDATATOKEN 13
#define INTEGERTOKEN         14

#define COMMENTTOKEN         32
#define KEYWORDTOKEN         33
#define IDENTIFIERTOKEN      34
#define ARGUMENTTOKEN        35
#define FLOATTOKEN           36
#define FIXEDPOINTTOKEN      37
#define BASEINTEGERTOKEN     38
#define PIPETOKEN            39
//#define RETURNTOKEN        40
#define OPERATORTOKEN        41
#define PERIODTOKEN          42
#define LPARENTOKEN          43
#define RPARENTOKEN          44
#define LCURLYTOKEN          45
#define RCURLYTOKEN          46
#define LBRACKETTOKEN        47
#define RBRACKETTOKEN        48




static char tokenBuffer[1024];
static char tokenBuffer2[1024];


// See the SimpleData documentation for the description of the file format
CompilerToken NewToken(MemStream ms)
{
    CompilerToken tok = celcalloc(sizeof(struct CompilerTokenStruct));
    tok->token = BADTOKEN;
    tok->file = ms;
    tok->start = 0;
    tok->end   = 0;
    return tok;
}


void freeToken(CompilerToken tok)
{
    celfree(tok);
}


void consumeToken(CompilerToken tok)
{
    consumed = 1;
}

// printToken - This routine is generally used for error displays
void printToken (CompilerToken tok)
{
    char buff[128];
    char buff2[128];
    uint32 startLine, startCol;
    uint32 endLine, endCol;
    uint32 i, j, k;
    uint32 cursor;
    uint32 start, end;
    
    getMSLineCol(tok->file, tok->start, &startLine, &startCol);
    getMSLineCol(tok->file, tok->end, &endLine, &endCol);
    // Just print the end line and column
    sprintf(buff," Token: [%d] Line: %ld Column: %ld\n", tok->token, (endLine+1), endCol);
    printString(buff);

    i = startLine;
    while (1) {
      getMSLine(tok->file, i, buff);
      printString(buff);

      start = 0;
      end   = strlen(buff);
      if (i == startLine) {
        start = startCol;
      }
      if (i == endLine) {
        end = endCol;
      }	

      cursor = j = 0;
      while (1) {
        if (cursor >= start) {
	  buff2[j] = '*';
        } else {
	  buff2[j] = ' ';
        }

	if (buff[cursor] == '\t')  {
	    for (k = (j + 1);k < (j + 8); k++) {
		buff2[k] = buff2[j];
	    }
	    j = k - 1;
	}
      
	j++;
	cursor++;
	
	if (cursor == end) {
	  buff2[j] = '\n';
	  j++;
	  buff2[j] = 0;
	  break;
        }
      }

      printString(buff2);

      i++;
      
      if (i > endLine) {
        break;
      }

    }
}

//
// The Start - Read a token in 
//             This is a wrapper that keeps track of the start and end
//             position
//
void getToken (CompilerToken tok)
{
    // If token not consumed, just return
    if (consumed == 0) { 
      return;
    }

    consumed = 0; 
    tok->start = tok->end;
    realGetToken(tok);
    tok->end = (int) positionMS(tok->file);
    if (tok->token == BADTOKEN) {
	tok->end++;		// Usually, the lexer stops at the start
	printError("Bad Token - ", tok);
    }
    
    return;
}

//
// The Real Get Token - this one does the work
//   Read a token in (ignoring whitespace)
//
void realGetToken (CompilerToken tok)
{

    int c;
    int i;
    uint32 l;
    
    while (1) {

	c = readMSCChar(tok->file);

	if (c == EOF) {
	    tok->token = ENDOFFILE;
	    return;
	}

	// Handle whitespace
	if ( (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f') ) {
	    tok->start += 1;
	    continue;
	}
	

	// DRUDRU - ignore comments is on
	if (c == '"') {
	    //try to read a comment
	    tok->token = COMMENTTOKEN;
	    readComment(tok);
	    if (ignoreComments) {
		tok->start = (int) positionMS(tok->file);
		continue;
	    }
	    else {
		return;
	    }
	}

	if (c == '/') {
	    //try to read a comment to end of line
	    c = readMSCChar(tok->file);
	    if (c != '/') {
		seekMSRelative(tok->file, -1);
		//reset c to what it was
		c = '/';
	    }
	    else {
		readComment2(tok);
		tok->start = (int) positionMS(tok->file);
		continue;
	    }
	}


	if (c == '\'' ) {
	    tok->token = STRINGTOKEN;
	    readString(tok);
	    return;
	}

	if (('_' == c) || (isalpha(c))) {
	    i = readIdentifierKeyword(tok);

	    if (i == 0) {
		printError("Lexer: Improper keyword or identifier", tok);
	    }
	    return;
	    
	}

	if (c == ':') {
	    i = readArgument(tok);
	    
	    if (i == 0) {
		printError("Lexer: Improper argument", tok);
	    }
	    return;
	    
	}

	if (c == '.') {
	    tok->token = PERIODTOKEN;
	    tokenBuffer[0] = '.';
	    tokenBuffer[1] = '\0';
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return;
	}

	if (c == '(') {
	    tok->token = LPARENTOKEN;
	    tokenBuffer[0] = '(';
	    tokenBuffer[1] = '\0';
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return;
	}

	if (c == ')') {
	    tok->token = RPARENTOKEN;
	    tokenBuffer[0] = ')';
	    tokenBuffer[1] = '\0';
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return;
	}

	if (c == '{') {
	    tok->token = LCURLYTOKEN;
	    tokenBuffer[0] = '{';
	    tokenBuffer[1] = '\0';
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return;
	}

	if (c == '}') {
	    tok->token = RCURLYTOKEN;
	    tokenBuffer[0] = '}';
	    tokenBuffer[1] = '\0';
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return;
	}

	if (c == '[') {
	    tok->token = LBRACKETTOKEN;
	    tokenBuffer[0] = '[';
	    tokenBuffer[1] = '\0';
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return;
	}

	if (c == ']') {
	    tok->token = RBRACKETTOKEN;
	    tokenBuffer[0] = ']';
	    tokenBuffer[1] = '\0';
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return;
	}

	// attempt to read all of the lexemes that have a possible op-char in front
	// numbers start with a - or a digit, no exceptions to this rule
	
	if ((c == '-') || isdigit(c)) {
	    tokenBuffer[0] = c;
	    tokenBuffer[1] = '\0';
	    
	    
	    i = 1;
	    
	    if (c == '-') {
		c = readMSCChar(tok->file);
		if (isdigit(c)) {
		    i = 1;
		}
		else {
		    // the '-' was an operator
		    i = 0;
		    // Reset 'c' to the '-' so it can be detected later on
		    c = '-';
		}
		seekMSRelative(tok->file, -1);
	    }
	    
	    // If c was a '-digit' or a digit, then we attempt to read a number
	    if (i) {
		l = positionMS(tok->file);
		l--;		// Offset back so the number can be read
		seekMS(tok->file, l);
		

		if (readFloat(tok)) {
		    return;
		}

		// Move back and try the next number type
		seekMS(tok->file, l);

		if (readFixedPoint(tok)) {
		    return;
		}

		// Move back and try the next number type
		seekMS(tok->file, l);
		
		if (readInteger(tok)) {
		    return;
		}

		printError("Lexer: Improper number type", tok);
	    }
	}
	
	if (
	    (c == '|') ||
	    (c == '!') ||
	    (c == '@') ||
	    (c == '#') ||
	    (c == '$') ||
	    (c == '%') ||
	    (c == '^') ||
	    (c == '&') ||
	    (c == '*') ||
	    (c == '-') ||
	    (c == '+') ||
	    (c == '=') ||
	    (c == '~') ||
	    (c == '/') ||
	    (c == '?') ||
	    (c == '<') ||
	    (c == '>') ||
	    (c == ',') ||
	    (c == ';') ||
	    (c == '|') ||
	    (c == '\\') ||
	    0 ) {
	    // try to read more operators
	    i = readOperator(tok);
	    return;
	}
	

	tok->token = BADTOKEN;
	
	return;
    }
    
}






    
void readComment(CompilerToken tok)
{
    int c;
    char * p;
    
    p = tokenBuffer;

    while (1) {
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    //// Set error - missing end " which started on line x
	    tok->token = BADTOKEN;
	    return;
	}
	if (c == '"') {
	    c = readMSCChar(tok->file);
	    if (c != '"') {	// if it isn't another quote
		// ..then it is the end of the comment

		// unget the character
		seekMSRelative(tok->file, -1);
		break;
	    } else {
		*p = c;
		p++;
	    }
	}
	*p = c;
	p++;
    }

    *p = 0;
    
    // Now convert to a a string proto
    tok->stringValue = (uint32) objectNewString(tokenBuffer);
}

void readComment2(CompilerToken tok)
{
    int c;
    
    while (1) {
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    return;
	}
	if (c == '\n') {
	    return;
	}
    }
}

    
void readString (CompilerToken tok)
{
    int c;
    char * p;
    char * p2;
    
    p = tokenBuffer;
    p2 = tokenBuffer2;

    while (1) {
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    //// Set error - missing end " which started on line x
	    tok->token = BADTOKEN;
	    return;
	}
	if (c == '\'') {
	    break;
	}

	// Read an escape char - and convert
	if (c == '\\') {
	    c = readMSCChar(tok->file);

	    switch (c) {
		case 't':
		    c = '\t';
		    break;
		    
		case 'b':
		    c = '\b';
		    break;
		    
		case 'n':
		    c = '\n';
		    break;
		    
		case 'f':
		    c = '\f';
		    break;
		    
		case 'r':
		    c = '\r';  
		    break;
		    
		case 'v':
		    c = '\v';  
		    break;
		    
		case 'a':
		    c = '\a';
		    break;
		    
		case '0':
		    c = '\0';  
		    break;
		    
		case '\\':
		    c = '\\';  
		    break;
		    
		case '\'':
		    c = '\'';  
		    break;
		    
		case '"':
		    c = '"';
		    break;

				// The dynamic cases
		case 'x':
		    p2 = tokenBuffer2;
		    *p2 = readMSCChar(tok->file);
		    if (!isxdigit(*p2)) {
			//// generate error
		    }
		    p2++;
		    *p2 = readMSCChar(tok->file);
		    if (!isxdigit(*p2)) {
			//// generate error
		    }
		    p2++;
		    *p2 = 0;
		    c = (uchar) strtol(tokenBuffer2, NULL, 16);
		    break;
		case 'd':
		    p2 = tokenBuffer2;
		    *p2 = readMSCChar(tok->file);
		    if (!isdigit(*p2)) {
			//// generate error
		    }
		    p2++;
		    *p2 = readMSCChar(tok->file);
		    if (!isdigit(*p2)) {
			//// generate error
		    }
		    p2++;
		    *p2 = readMSCChar(tok->file);
		    if (!isdigit(*p2)) {
			//// generate error
		    }
		    p2++;
		    *p2 = 0;
		    c = (uchar) strtol(tokenBuffer2, NULL, 10);
		    break;
		case 'o':
		    p2 = tokenBuffer2;
		    *p2 = readMSCChar(tok->file);
		    if (!isdigit(*p2)) {
			//// generate error
		    }
		    p2++;
		    *p2 = readMSCChar(tok->file);
		    if (!isdigit(*p2)) {
			//// generate error
		    }
		    p2++;
		    *p2 = readMSCChar(tok->file);
		    if (!isdigit(*p2)) {
			//// generate error
		    }
		    p2++;
		    *p2 = 0;
		    c = (uchar) strtol(tokenBuffer2, NULL, 8);
		    break;
	    }
	}
	*p = c;
	p++;
    }

    *p = 0;
    
    // Now convert to a a string proto
    tok->stringValue = (uint32) objectNewString(tokenBuffer);
}


int readIdentifierKeyword(CompilerToken tok)
{
    int c;
    char * p;
    char * p2;
    
    p = tokenBuffer;
    p2 = tokenBuffer2;

    // Jump back one character
    seekMSRelative(tok->file, -1);

    while (1) {
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    //// Set error - missing end " which started on line x
	    tok->token = BADTOKEN;
	    return 0;
	}
	if (isalnum(c) || (c == '_')) {
	} else {
	    break;
	}

	*p = c;
	p++;
    }

    if (c == ':') {
	tok->token = KEYWORDTOKEN;
	*p = c;
	p++;
    } else {
	tok->token = IDENTIFIERTOKEN;
	seekMSRelative(tok->file, -1);
    }
    
    *p = 0;
    
    // Now convert to a a string proto
    tok->stringValue = (uint32) objectNewString(tokenBuffer);
    return 1;
    
}



int readArgument(CompilerToken tok)
{
    int c;
    char * p;
    char * p2;
    
    p = tokenBuffer;
    p2 = tokenBuffer2;

    // Jump back one character
    *p = ':';
    p++;
    c = readMSCChar(tok->file);
    if (c == EOF) {
	//// Set error - missing end " which started on line x
	tok->token = BADTOKEN;
	return 0;
    }
    if (isalpha(c) || (c == '_')) {
    } else {
	//// Set error - missing end " which started on line x
	tok->token = BADTOKEN;
	return 0;
    }
    *p = c;
    p++;
    while (1) {
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    //// Set error - missing end " which started on line x
	    tok->token = BADTOKEN;
	    return 0;
	}
	if (isalnum(c) || (c == '_')) {
	} else {
	    seekMSRelative(tok->file, -1);
	    break;
	}

	*p = c;
	p++;
    }

    tok->token = ARGUMENTTOKEN;
    *p = 0;
    
    // Now convert to a a string proto
    tok->stringValue = (uint32) objectNewString(tokenBuffer);
    return 1;
    
}



int readOperator (CompilerToken tok)
{
    int c;
    char * p;
    char * p2;
    
    p = tokenBuffer;
    p2 = tokenBuffer2;

    // Jump back one character
    seekMSRelative(tok->file, -1);

    while (1) {
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    //// Set error - missing end " which started on line x
	    tok->token = BADTOKEN;
	    return 0;
	}
	if (
	    (c == '|') ||
	    (c == '!') ||
	    (c == '@') ||
	    (c == '#') ||
	    (c == '$') ||
	    (c == '%') ||
	    (c == '^') ||
	    (c == '&') ||
	    (c == '*') ||
	    (c == '-') ||
	    (c == '+') ||
	    (c == '=') ||
	    (c == '~') ||
	    (c == '/') ||
	    (c == '?') ||
	    (c == '<') ||
	    (c == '>') ||
	    (c == ',') ||
	    (c == ';') ||
	    (c == '|') ||
	    (c == '\\') ||
	    0 ) {
	    *p = c;
	    p++;
	} else {
	    // Undo the read and stop
	    seekMSRelative(tok->file, -1);
	    break;
	}
    }

    tok->token = OPERATORTOKEN;
    *p = 0;

    // Now convert to a a string proto
    tok->stringValue = (uint32) objectNewString(tokenBuffer);

    if (tokenBuffer[0] == '|' && tokenBuffer[1] == '\0') {
	tok->token = PIPETOKEN;
    }
    /*
    else
    if (tokenBuffer[0] == '^' && tokenBuffer[1] == '\0') {
	tok->token = RETURNTOKEN;
    }
    */
    
    return 1;
    
}



int readFloat (CompilerToken tok)
{
    int c;
    char * p;
    char * p2;
    int flag = 0;
    
    
    p = tokenBuffer;
    p2 = tokenBuffer2;

    // Read first character, '-' or digit
    c = readMSCChar(tok->file);
    if (c == EOF) {
	//// Set error - missing end " which started on line x
	tok->token = BADTOKEN;
	return 0;
    }
    if (c == '-') {
	*p = c;
	p++;
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    //// Set error - missing end " which started on line x
	    tok->token = BADTOKEN;
	    return 0;
	}
    }

    //// NEED TO CHECK TO MAKE SURE AT LEAST ONE IS READ
    while (1) {
	if (isdigit(c)) {
	    *p = c;
	    p++;
	    c = readMSCChar(tok->file);
	} else {
	    break;
	}
	
    }
    
    // Read fractional part
    if (c == '.') {
	*p = c;
	p++;
	c = readMSCChar(tok->file);

	if (isdigit(c)) {
	    // This is a float
	    flag = 1;
	    
	    *p = c;
	    p++;
	    c = readMSCChar(tok->file);
	    
	    while (isdigit(c)) {
		*p = c;
		p++;
		c = readMSCChar(tok->file);
	    }
	} else {
	    return 0;
	}
    }

    if ( (c == 'e') || (c == 'E') ) {
	*p = c;
	p++;
	
	c = readMSCChar(tok->file);

	if ( (c == '+') || (c == '-') ){
	    *p = c;
	    p++;
	    c = readMSCChar(tok->file);
	}

	if (isdigit(c)) {
	    *p = c;
	    p++;
	    c = readMSCChar(tok->file);

	    while (isdigit(c)) {
		*p = c;
		p++;
		c = readMSCChar(tok->file);
	    }
	}
    }

    if (flag) {
	// Undo 1 character
	seekMSRelative(tok->file, -1);
	tok->token = FLOATTOKEN;
	*p = 0;
	
	// Now convert to a a string proto
	tok->stringValue = (uint32) objectNewString(tokenBuffer);
	return 1;
    }
    
    return 0;
}

	
int readFixedPoint(CompilerToken tok)
{
    int c;
    char * p;
    char * p2;
    
    p = tokenBuffer;
    p2 = tokenBuffer2;

    // Read first character, '-' or digit
    c = readMSCChar(tok->file);
    if (c == EOF) {
	//// Set error - missing end " which started on line x
	tok->token = BADTOKEN;
	return 0;
    }
    if (c == '-') {
	*p = c;
	p++;
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    //// Set error - missing end " which started on line x
	    tok->token = BADTOKEN;
	    return 0;
	}
    }

    //// NEED TO CHECK TO MAKE SURE AT LEAST ONE IS READ
    while (1) {
	if (isdigit(c)) {
	    *p = c;
	    p++;
	    c = readMSCChar(tok->file);
	} else {
	    break;
	}
    }
    
    // Read fractional part
    if (c == '.') {
	*p = c;
	p++;
	c = readMSCChar(tok->file);

	if (isdigit(c)) {
	    *p = c;
	    p++;
	    c = readMSCChar(tok->file);
	    
	    while (isdigit(c)) {
		*p = c;
		p++;
		c = readMSCChar(tok->file);
	    }

	    // Undo one character
	    seekMSRelative(tok->file, -1);
	    
	    tok->token = FIXEDPOINTTOKEN;
	    *p = 0;
	    
	    // Now convert to a a string proto
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return 1;
	    
	} else {
	    return 0;
	}
    }

    return 0;
}



int readInteger(CompilerToken tok)
{
    int c;
    char * p;
    char * p2;
    uint32 l;
    
    
    p = tokenBuffer;
    p2 = tokenBuffer2;

    
    // Read first character, '-' or digit
    c = readMSCChar(tok->file);
    if (c == EOF) {
	//// Set error - missing end " which started on line x
	tok->token = BADTOKEN;
	return 0;
    }
    if (c == '-') {
	*p = c;
	p++;
	c = readMSCChar(tok->file);
	if (c == EOF) {
	    //// Set error - missing end " which started on line x
	    tok->token = BADTOKEN;
	    return 0;
	}
    }

    //// READ BASE
    //// READ GENERAL DIGITS
    l = positionMS(tok->file);
    
    if (isdigit(c)) {
	*p = c;
	p++;
	c = readMSCChar(tok->file);

	while (1) {
	    if (isdigit(c)) {
		*p = c;
		p++;
		c = readMSCChar(tok->file);
	    } else {
		break;
	    }
	}

	if ((c == 'r') || (c == 'R')) {
	    // Read the general digits
	    *p = c;
	    p++;
	    c = readMSCChar(tok->file);

	    
	    while (1) {
		if (isxdigit(c)) { // NOTE the isXdigit is not isdigit
		    *p = c;
		    p++;
		    c = readMSCChar(tok->file);
		} else {
		    break;
		}
	    }

            // Undo one character
	    seekMSRelative(tok->file, -1);
	    
	    tok->token = BASEINTEGERTOKEN;
	    *p = 0;
	    
	    // Now convert to a a string proto
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return 1;

	} else {
	    // Undo one character
	    seekMSRelative(tok->file, -1);
	    
	    tok->token = INTEGERTOKEN;
	    *p = 0;
	    
	    // Now convert to a a string proto
	    tok->stringValue = (uint32) objectNewString(tokenBuffer);
	    return 1;
	}
    }

    return 0;
    
}





void *getValue(CompilerToken tok)
{
    return (void *)tok->stringValue;
}


char isBadToken(CompilerToken tok)
{
    return (tok->token == BADTOKEN);
}

char isEndOfFile(CompilerToken tok)
{
    return (tok->token == ENDOFFILE);
}

char isCommentToken(CompilerToken tok)
{
    return (tok->token == COMMENTTOKEN);
}

char isStringToken(CompilerToken tok)
{
    return (tok->token == STRINGTOKEN);
}

char isIdentifierToken(CompilerToken tok)
{
    return (tok->token == IDENTIFIERTOKEN);
}

char isKeywordToken(CompilerToken tok)
{
    return (tok->token == KEYWORDTOKEN);
}

char isArgumentToken(CompilerToken tok)
{
    return (tok->token == ARGUMENTTOKEN);
}

char isOperatorToken(CompilerToken tok)
{
    return (tok->token == OPERATORTOKEN);
}

char isPipeToken(CompilerToken tok)
{
    return (tok->token == PIPETOKEN);
}
/*
char isReturnToken(Token tok)
{
    return (tok->token == RETURNTOKEN);
}
*/

char isFloatToken(CompilerToken tok)
{
    return (tok->token == FLOATTOKEN);
}

char isFixedPointToken(CompilerToken tok)
{
    return (tok->token == FIXEDPOINTTOKEN);
}

char isIntegerToken(CompilerToken tok)
{
    return (tok->token == INTEGERTOKEN);
}

char isBaseIntegerToken(CompilerToken tok)
{
    return (tok->token == BASEINTEGERTOKEN);
}

char isPeriodToken(CompilerToken tok)
{
    return (tok->token == PERIODTOKEN);
}

char isLParenToken(CompilerToken tok)
{
    return (tok->token == LPARENTOKEN);
}

char isRParenToken(CompilerToken tok)
{
    return (tok->token == RPARENTOKEN);
}

char isLCurlyToken(CompilerToken tok)
{
    return (tok->token == LCURLYTOKEN);
}

char isRCurlyToken(CompilerToken tok)
{
    return (tok->token == RCURLYTOKEN);
}

char isLBracketToken(CompilerToken tok)
{
    return (tok->token == LBRACKETTOKEN);
}

char isRBracketToken(CompilerToken tok)
{
    return (tok->token == RBRACKETTOKEN);
}

