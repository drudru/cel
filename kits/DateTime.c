
//
// DateTime.c
//
// <see corresponding .h file>
// 
//
// $Id: DateTime.c,v 1.8 2002/02/22 22:23:24 dru Exp $
//
//
//
//

#include "cel.h"
#include "runtime.h"
#include "svm32.h"
#include <time.h>
#include <string.h>

#include "config.h"


//
//  DateTime - this is the wrapper over Unix for Cel
// 

extern char *DateTimeSL[];


// This takes the _dateTime blob and resets all of the fields
// This assumes that the _dateTime and parent slots are set
static void DateTimeRefresh(Proto proto);

// This takes the time_t structure and creates a time object
static Proto DateTimeCreate(time_t tm);



// Factory methods
void DateTimeNow(void)
{
    Proto  proto;
    time_t tm;

    time(&tm);

    proto = DateTimeCreate(tm);

    VMReturn(Cpu, (unsigned int) proto, 3);
}


void DateTimeToday(void)
{
    Proto  proto;
    time_t tm;
    struct tm * tptr;
    
    time(&tm);
    tptr = localtime(&tm);
    
    tptr->tm_sec = tptr->tm_min = tptr->tm_hour = 0;
    tm = mktime(tptr);

    proto = DateTimeCreate(tm);

    VMReturn(Cpu, (unsigned int) proto, 3);
}


void DateTimeThisMonth(void)
{
    Proto  proto;
    time_t tm;
    struct tm * tptr;
    
    time(&tm);
    tptr = localtime(&tm);
    
    tptr->tm_sec = tptr->tm_min = tptr->tm_hour = 0;
    tptr->tm_mday = 1;
    tm = mktime(tptr);

    proto = DateTimeCreate(tm);

    VMReturn(Cpu, (unsigned int) proto, 3);
}

// End of Factory methods






void DateTimeAddDays(void)
{
    Proto  proto;
    Proto  tmp;
    Proto  blob;
    int    i;
    time_t * tmPtr;
    
    // Get the number of days to add
    tmp = (Proto) stackAt(Cpu, 4);
    i = objectIntegerValue(tmp);
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, ATOM_datetime, &blob);

    tmPtr = (time_t *)objectPointerValue(blob);

    (*tmPtr) += i * 86400;

    DateTimeRefresh(proto);

    VMReturn(Cpu, (unsigned int) proto, 4);
}


void DateTimeAddMonths(void)
{
    Proto  proto;
    Proto  tmp;
    Proto  blob;
    int    i;
    time_t * tmPtr;
    struct tm * tptr;
    
    // Get the number of days to add
    tmp = (Proto) stackAt(Cpu, 4);
    i = objectIntegerValue(tmp);
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, ATOM_datetime, &blob);

    tmPtr = (time_t *)objectPointerValue(blob);

    // Convert to a structure
    tptr = localtime(tmPtr);
    
    tptr->tm_mon += i;
    (*tmPtr) = mktime(tptr);

    DateTimeRefresh(proto);

    VMReturn(Cpu, (unsigned int) proto, 4);
}



void DateTimeSubtract(void)
{
    Proto  proto;
    Proto  tmp;
    Proto  blob;
    int    i;
    time_t * tmPtr1;
    time_t * tmPtr2;
    
    // Get the number of days to add
    tmp = (Proto) stackAt(Cpu, 4);
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, ATOM_datetime, &blob);
    tmPtr1 = (time_t *)objectPointerValue(blob);

    objectGetSlot(tmp, ATOM_datetime, &blob);
    tmPtr2 = (time_t *)objectPointerValue(blob);

    i = (*tmPtr1) - (*tmPtr2);

    proto = DateTimeCreate(i);

    VMReturn(Cpu, (unsigned int) proto, 4);
}


void DateTimeAsSeconds(void)
{
    Proto  proto;
    Proto  blob;
    int    i;
    time_t * tmPtr;
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, ATOM_datetime, &blob);

    tmPtr = (time_t *)objectPointerValue(blob);

    i = (*tmPtr);

    VMReturn(Cpu, (unsigned int) objectNewInteger(i), 3);
}



// This takes the _dateTime blob and resets all of the fields
// This assumes that the _dateTime and parent slots are set
void DateTimeRefresh(Proto proto)
{
    Proto  blob;
    char * p;
    time_t tm;
    struct tm * tptr;
    
    
    objectGetSlot(proto, ATOM_datetime, &blob);
    p = objectPointerValue(blob);
    memcpy(&tm, p, sizeof(tm));

    tptr = localtime(&tm);
    
    objectSetSlot(proto, stringToAtom("seconds"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_sec));
    objectSetSlot(proto, stringToAtom("minutes"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_min));
    objectSetSlot(proto, stringToAtom("hours"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_hour));
    objectSetSlot(proto, stringToAtom("day"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_mday));
    objectSetSlot(proto, stringToAtom("month"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_mon + 1));
    objectSetSlot(proto, stringToAtom("year"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_year + 1900));
    objectSetSlot(proto, stringToAtom("weekDay"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_wday));
    objectSetSlot(proto, stringToAtom("dayOfYear"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_yday));

    objectSetSlot(proto, stringToAtom("isDaylightSavings"), PROTO_ACTION_GET, (uint32) (tptr->tm_isdst ? (TrueObj) : (FalseObj)));
#ifdef unixbsd
    objectSetSlot(proto, stringToAtom("timeZoneName"), PROTO_ACTION_GET, (uint32) stringToAtom((char *)tptr->tm_zone));
    objectSetSlot(proto, stringToAtom("timeZoneSecondsOffset"), PROTO_ACTION_GET, (uint32) objectNewInteger(tptr->tm_gmtoff));
#else
    objectSetSlot(proto, stringToAtom("timeZoneName"), PROTO_ACTION_GET, (uint32) stringToAtom((char *)tzname[1]));
    objectSetSlot(proto, stringToAtom("timeZoneSecondsOffset"), PROTO_ACTION_GET, (uint32) objectNewInteger(timezone));
#endif

}


// This takes the time_t structure and creates a time object
Proto DateTimeCreate(time_t tm)
{
    Proto  proto;
    Proto  blob;
    Proto  parent;
    char * p;

    GCOFF();
    
    proto = objectNew(14);

    blob = objectNewBlob(sizeof(time_t));
    p = objectPointerValue(blob);
    
    memcpy(p, &tm, sizeof(tm));
    
    objectSetSlot(proto, ATOM_datetime, PROTO_ACTION_GET, (uint32) blob);

    objectGetSlot(Globals, stringToAtom("DateTime"), &parent);
    objectSetSlot(proto, ATOMparent, PROTO_ACTION_GET, (uint32) parent);

    // This sets up all the slots based on the time_t blob
    DateTimeRefresh(proto);

    GCON();
    
    return proto;
}



void DateTimeAsDays(void)
{
    Proto  proto;
    Proto  result;
    Proto  blob;
    time_t * tmPtr;
    int    days;
    
    
    proto = (Proto) stackAt(Cpu, 2);

    objectGetSlot(proto, ATOM_datetime, &blob);

    tmPtr = (time_t *)objectPointerValue(blob);

    days = *tmPtr / 86400;

    result = objectNewInteger(days);

    VMReturn(Cpu, (unsigned int) result, 3);
}

// This shouldn't be used
// Why? it shouldn't be possible to set those
// fields in a real time object
void DateTimeInternalize(void)
{
    Proto  proto;
    Proto  blob;
    Proto  tmp;
    char * p;
    time_t tm;
    struct tm timeRec;
    

    // Clear the timeRec
    memset(&timeRec, 0, sizeof(struct tm));
    

    proto = (Proto) stackAt(Cpu, 2);
    

    // Get the values out of the proto
    objectGetSlot(proto, stringToAtom("seconds"), &tmp);
    timeRec.tm_sec = objectIntegerValue(tmp);
    objectGetSlot(proto, stringToAtom("minutes"), &tmp);
    timeRec.tm_min = objectIntegerValue(tmp);
    objectGetSlot(proto, stringToAtom("hours"), &tmp);
    timeRec.tm_hour = objectIntegerValue(tmp);
    objectGetSlot(proto, stringToAtom("day"), &tmp);
    timeRec.tm_mday = objectIntegerValue(tmp);
    objectGetSlot(proto, stringToAtom("month"), &tmp);
    timeRec.tm_mon = objectIntegerValue(tmp) - 1;
    objectGetSlot(proto, stringToAtom("year"), &tmp);
    timeRec.tm_year = objectIntegerValue(tmp) - 1900;

    // Convert to time_t and store that in the _dateTime blob
    tm = mktime(&timeRec);
    objectGetSlot(proto, ATOM_datetime, &blob);
    p = objectPointerValue(blob);
    memcpy(p, &tm, sizeof(tm));

    DateTimeRefresh(proto);

    VMReturn(Cpu, (unsigned int) proto, 3);
}



void DateTimeYMDHMS(void)
{
    Proto  proto;
    Proto  tmp;
    time_t tm;
    struct tm timeRec;
    

    // Clear the timeRec
    memset(&timeRec, 0, sizeof(struct tm));

    proto = (Proto) stackAt(Cpu, 2);

    // Get the values out of the proto

    tmp = (Proto) stackAt(Cpu, 4);
    timeRec.tm_year = objectIntegerValue(tmp) - 1900;

    tmp = (Proto) stackAt(Cpu, 5);
    timeRec.tm_mon = objectIntegerValue(tmp) - 1;

    tmp = (Proto) stackAt(Cpu, 6);
    timeRec.tm_mday = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 7);
    timeRec.tm_hour = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 8);
    timeRec.tm_min = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 9);
    timeRec.tm_sec = objectIntegerValue(tmp);

    // Assume that the time is in proper daylight savings time
    timeRec.tm_isdst = -1;
    
    // Convert to time_t and store that in the _dateTime blob
    tm = mktime(&timeRec);

    proto = DateTimeCreate(tm);
    
    VMReturn(Cpu, (unsigned int) proto, 9);
}

char *DateTimeSL[] = {
	"now",    "DateTimeNow", (char *) DateTimeNow,
	"today",  "DateTimeToday", (char *) DateTimeToday,
	"thisMonth",  "DateTimeThisMonth", (char *) DateTimeThisMonth,
	"addDays:", "DateTimeAddDays", (char *) DateTimeAddDays,
	"addMonths:", "DateTimeAddMonths", (char *) DateTimeAddMonths,
	"-", "DateTimeSubtract", (char *) DateTimeSubtract,
	"asSeconds", "DateTimeAsSeconds", (char *) DateTimeAsSeconds,
	"asDays", "DateTimeAsDays", (char *) DateTimeAsDays,
	"internalize", "DateTimeInternalize", (char *) DateTimeInternalize,
	"fromYear:Month:Day:Hours:Minutes:Seconds:", "DateTimeYMDHMS", (char *) DateTimeYMDHMS,
	""
};
