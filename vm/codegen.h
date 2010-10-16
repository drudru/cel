
//
// codegen.h
//
// This has the parse tree transformation routines and the code
// generation routines.
//
//
// $Id: codegen.h,v 1.3 2001/01/05 00:58:54 dru Exp $
//
//



#ifndef _CEL_CODEGEN_H
#define _CEL_CODEGEN_H 1


Proto transform(Proto parseTree);

Proto codeGen(Proto parseTree, int addStart);


#endif

