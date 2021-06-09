//---------------------------------------------------------------------------

#ifndef logH
#define logH

#include <Quick.Logger.hpp>
#include <Quick.Logger.Provider.Files.hpp>
#pragma hdrstop
//---------------------------------------------------------------------------

#define LOG_LEVEL0(level, msg) \
do { \
	Quick::Logger::Logger->level(msg); \
} while(false)

#define LOG_LEVEL1(level, msg, p1) \
do { \
	TVarRec args[] = { (p1), }; \
	Quick::Logger::Logger->level(msg, args, 1); \
} while(false)

#define LOG_LEVEL2(level, msg, p1, p2) \
do { \
	TVarRec args[] = { (p1), (p2), }; \
	Quick::Logger::Logger->level(msg, args, 2); \
} while(false)

#define LOG_LEVEL3(level, msg, p1, p2, p3) \
do { \
	TVarRec args[] = { (p1), (p2), (p3), }; \
	Quick::Logger::Logger->level(msg, args, 3); \
} while(false)

#define LOG_LEVEL4(level, msg, p1, p2, p3, p4) \
do { \
	TVarRec args[] = { (p1), (p2), (p3), (p4), }; \
	Quick::Logger::Logger->level(msg, args, 4); \
} while(false)

#define LOG_DEBUG0(msg) LOG_LEVEL0(Debug, msg)
#define LOG_DEBUG1(msg, p1) LOG_LEVEL1(Debug, msg, p1)
#define LOG_DEBUG2(msg, p1, p2) LOG_LEVEL2(Debug, msg, p1, p2)
#define LOG_DEBUG3(msg, p1, p2, p3) LOG_LEVEL3(Debug, msg, p1, p2, p3)
#define LOG_DEBUG4(msg, p1, p2, p3, p4) LOG_LEVEL4(Debug, msg, p1, p2, p3, p4)

#define LOG_INFO0(msg) LOG_LEVEL0(Info, msg)
#define LOG_INFO1(msg, p1) LOG_LEVEL1(Info, msg, p1)
#define LOG_INFO2(msg, p1, p2) LOG_LEVEL2(Info, msg, p1, p2)
#define LOG_INFO3(msg, p1, p2, p3) LOG_LEVEL3(Info, msg, p1, p2, p3)
#define LOG_INFO4(msg, p1, p2, p3, p4) LOG_LEVEL4(Info, msg, p1, p2, p3, p4)

#define LOG_DONE0(msg) LOG_LEVEL0(Done, msg)
#define LOG_DONE1(msg, p1) LOG_LEVEL1(Done, msg, p1)
#define LOG_DONE2(msg, p1, p2) LOG_LEVEL2(Done, msg, p1, p2)
#define LOG_DONE3(msg, p1, p2, p3) LOG_LEVEL3(Done, msg, p1, p2, p3)
#define LOG_DONE4(msg, p1, p2, p3, p4) LOG_LEVEL4(Done, msg, p1, p2, p3, p4)

#define LOG_WARNING0(msg) LOG_LEVEL0(Warn, msg)
#define LOG_WARNING1(msg, p1) LOG_LEVEL1(Warn, msg, p1)
#define LOG_WARNING2(msg, p1, p2) LOG_LEVEL2(Warn, msg, p1, p2)
#define LOG_WARNING3(msg, p1, p2, p3) LOG_LEVEL3(Warn, msg, p1, p2, p3)
#define LOG_WARNING4(msg, p1, p2, p3, p4) LOG_LEVEL4(Warn, msg, p1, p2, p3, p4)

#define LOG_ERROR0(msg) LOG_LEVEL0(Error, msg)
#define LOG_ERROR1(msg, p1) LOG_LEVEL1(Error, msg, p1)
#define LOG_ERROR2(msg, p1, p2) LOG_LEVEL2(Error, msg, p1, p2)
#define LOG_ERROR3(msg, p1, p2, p3) LOG_LEVEL3(Error, msg, p1, p2, p3)
#define LOG_ERROR4(msg, p1, p2, p3, p4) LOG_LEVEL4(Error, msg, p1, p2, p3, p4)

#define LOG_CRITICAL0(msg) LOG_LEVEL0(Critical, msg)
#define LOG_CRITICAL1(msg, p1) LOG_LEVEL1(Critical, msg, p1)
#define LOG_CRITICAL2(msg, p1, p2) LOG_LEVEL2(Critical, msg, p1, p2)
#define LOG_CRITICAL3(msg, p1, p2, p3) LOG_LEVEL3(Critical, msg, p1, p2, p3)
#define LOG_CRITICAL4(msg, p1, p2, p3, p4) LOG_LEVEL4(Critical, msg, p1, p2, p3, p4)

//---------------------------------------------------------------------------
#endif
