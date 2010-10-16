
//
// assemble.h
//
// This assembles code that is in proto form to a code proto.
// It takes a slot that has the source and then it turns that
// into a passthru to the code proto that houses the code.
//
//
// $Id: assemble.h,v 1.4 2001/01/05 00:58:54 dru Exp $
//
//
//
//
//
//



#ifndef _CEL_ASSEMBLE_H
#define _CEL_ASSEMBLE_H 1


// assemble - routines


Proto   assemble(Proto proto, Atom slot, int isBlock);

Proto   assembleAll(Proto proto);

char *  assembleError(void);

#endif


