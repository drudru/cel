
//
// GD.c
//
// <see corresponding .h file>
// 
//
// $Id: GD.c,v 1.4 2002/02/22 18:45:44 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "svm32.h"
#include "gd.h"
#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"


//
//  GD - this is the wrapper over www.boutell.com GD for Cel
// 

extern char *GDSL[];

// Factory methods


void GDcreateWidthHeight (void)
{
    Proto  proto;
    Proto  tmp;
    Proto  blob;
    Proto  parent;
    gdImagePtr imgPtr;
    char * p;
    int w,h;
    

    tmp = (Proto) stackAt(Cpu, 4);
    w = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 5);
    h = objectIntegerValue(tmp);

    imgPtr = gdImageCreate(w, h);

    
    GCOFF();
    
    proto = objectNew(0);

    blob = objectNewBlob(sizeof(gdImagePtr));
    p    = objectPointerValue(blob);
    
    memcpy(p, &imgPtr, sizeof(gdImagePtr));

    objectGetSlot(Globals, stringToAtom("GD"), &parent);
    objectSetSlot(proto, ATOMparent, PROTO_ACTION_GET, (uint32) parent);

    objectSetSlot(proto, stringToAtom("_gdp"), PROTO_ACTION_GET, (uint32) blob);
    
    GCON();

    
    VMReturn(Cpu, (unsigned int) proto, 5);
}



void GDallocColorRGB(void)
{
    Proto  proto;
    Proto  tmp;
    Proto  blob;
    gdImagePtr imgPtr;
    char * p;
    uchar  r,g,b;
    int    index;
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_gdp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&imgPtr, p, sizeof(imgPtr));
    
    tmp = (Proto) stackAt(Cpu, 4);
    r = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 5);
    g = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 6);
    b = objectIntegerValue(tmp);

    index = gdImageColorAllocate(imgPtr, r, g, b);

    VMReturn(Cpu, (unsigned int) objectNewInteger(index), 6);
}


void GDallocColorTransparent(void)
{
    Proto  proto;
    Proto  tmp;
    Proto  blob;
    gdImagePtr imgPtr;
    char * p;
    uchar  t;
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_gdp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&imgPtr, p, sizeof(imgPtr));
    
    tmp = (Proto) stackAt(Cpu, 4);
    t = objectIntegerValue(tmp);

    gdImageColorTransparent(imgPtr, t);

    VMReturn(Cpu, (unsigned int) proto, 4);
}



void GDlineX1Y1X2Y2Color(void)
{
    Proto  proto;
    Proto  tmp;
    Proto  blob;
    gdImagePtr imgPtr;
    char * p;
    int    x1,y1,x2,y2;
    int    index;
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_gdp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&imgPtr, p, sizeof(imgPtr));
    
    tmp = (Proto) stackAt(Cpu, 4);
    x1 = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 5);
    y1 = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 6);
    x2 = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 7);
    y2 = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 8);
    index = objectIntegerValue(tmp);

    gdImageLine(imgPtr, x1, y1, x2, y2, index);

    VMReturn(Cpu, (unsigned int) objectNewInteger(index), 8);
}



gdFontPtr getFontPtr(int font)
{
    switch (font) {
	case 1:
	    return gdFontTiny;
	case 2:
	    return gdFontSmall;
	case 3:
	    return gdFontMediumBold;
	case 4:
	    return gdFontLarge;
	case 5:
	    return gdFontGiant;
	default:
	    return gdFontTiny;
    }
}



void GDstringwithFontXYColor(void)
{
    Proto  proto;
    Proto  tmp;
    Proto  blob;
    Proto  strObj;
    gdImagePtr imgPtr;
    gdFontPtr  fntPtr;
    char * p;
    int    x1,y1;
    int    font;
    int    index;
    char * buff;
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_gdp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&imgPtr, p, sizeof(imgPtr));
    
    tmp = (Proto) stackAt(Cpu, 4);
    strObj = tmp;

    tmp = (Proto) stackAt(Cpu, 5);
    font= objectIntegerValue(tmp);
    fntPtr = getFontPtr(font);

    tmp = (Proto) stackAt(Cpu, 6);
    x1 = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 7);
    y1 = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 8);
    index = objectIntegerValue(tmp);

    buff = string2CString(strObj);
    gdImageString(imgPtr, fntPtr, x1, y1, buff, index);
    celfree(buff);
    
    VMReturn(Cpu, (unsigned int) proto, 8);
}



void GDfree(void)
{
    Proto  proto;
    Proto  blob;
    gdImagePtr imgPtr;
    char * p;
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_gdp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&imgPtr, p, sizeof(imgPtr));
    
    gdImageDestroy(imgPtr);

    VMReturn(Cpu, (unsigned int) nil, 3);
}


void GDasStringPNG(void)
{
    Proto  proto;
    Proto  blob;
    Proto  newObj;
    gdImagePtr imgPtr;
    char * p;
    char * buff;
    int    size;
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, stringToAtom("_gdp"), &blob);
    p = (char *) objectPointerValue(blob);
    memcpy(&imgPtr, p, sizeof(imgPtr));

    buff = gdImagePngPtr(imgPtr, &size);
    newObj = createString(buff, size);
    gdFree(buff);

    VMReturn(Cpu, (unsigned int) newObj, 3);
}


char *GDSL[] = {
	"createWidth:Height:", "GDcreateWidthHeight", (char *) GDcreateWidthHeight,
	"allocColorR:G:B:", "GDallocColorRGB", (char *) GDallocColorRGB,
	"allocColorTransparent:", "GDallocColorTransparent", (char *) GDallocColorTransparent,
	"lineX1:Y1:X2:Y2:Color:", "GDlineX1Y1X2Y2Color", (char *) GDlineX1Y1X2Y2Color,
	"string:withFont:X:Y:Color:", "GDstringwithFontXYColor", (char *) GDstringwithFontXYColor,
	"free", "GDfree", (char *) GDasStringPNG,
	"asStringPNG", "GDasStringPNG", (char *) GDasStringPNG,
	""
};

