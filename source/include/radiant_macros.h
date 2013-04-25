#ifndef _RADIANT_MACROS_H
#define _RADIANT_MACROS_H

#define OFFSET_OF(t, f)    (int)(&((t *)(NULL))->f)
#define ARRAY_SIZE(a)      ((sizeof (a)) / sizeof((a)[0]))

#define ASSERT(x)          do { if (!(x)) { DebugBreak(); } } while(false)
#define NYI_ERROR_CHECK(x) ASSERT(x)

#define DEBUG_ONLY(x)      do { x } while (false);
#define RADIANT_NOT_IMPLEMENTED()   (LOG(WARNING) << __FUNCTION__  << " not implemented (" << __FILE__ << ":" << __LINE__)
#define NOT_YET_IMPLEMENTED()       (LOG(WARNING) << __FUNCTION__  << " not implemented (" << __FILE__ << ":" << __LINE__)
#define NOT_REACHED()      ASSERT(false)

#define NO_COPY_CONSTRUCTOR(Cls) \
   Cls(const Cls& other); \
   const Cls& operator=(const Cls& rhs);

#endif // _RADIANT_MACROS_H
