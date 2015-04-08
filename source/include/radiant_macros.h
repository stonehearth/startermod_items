#ifndef _RADIANT_MACROS_H
#define _RADIANT_MACROS_H

#include "radiant_assert.h"
#include <sstream>

#define RADIANT_OPT_LEVEL_DEV       0
#define RADIANT_OPT_LEVEL_ALPHA     1
#define RADIANT_OPT_LEVEL_BETA      2
#define RADIANT_OPT_LEVEL_RC        3
#define RADIANT_OPT_LEVEL_RELEASE   4

#if !defined(RADIANT_OPT_LEVEL)
#  define RADIANT_OPT_LEVEL RADIANT_OPT_LEVEL_DEV
#endif

#if defined(_M_X64) || defined(__amd64__)
#define RADIANT_X64     1
#endif

#define OFFSET_OF(t, f)    (int)(&((t *)(NULL))->f)
#define ARRAY_SIZE(a)      ((sizeof (a)) / sizeof((a)[0]))

#define BEGIN_RADIANT_NAMESPACE  namespace radiant {
#define END_RADIANT_NAMESPACE    }

#if defined(ASSERT)
#  error ASSERT defined before inclusion of radiant_macros.h.  Aborting
#endif

#if defined(NO_ASSERT)
#  define ASSERT(x)
#else
#  define ASSERT(x) \
      do { \
         if (!(x)) { \
         radiant::HandleAssert(BUILD_STRING("Assertion Failed: " #x  "(" __FILE__ ":" << __LINE__ << ")")); \
         } \
      } while(false)
#endif

#define BUILD_STRING(x) static_cast<std::ostringstream&>(std::ostringstream() << x).str()

#if defined(_DEBUG)
#  define DEBUG_ONLY(x)      do { x } while (false);
#else
#  define DEBUG_ONLY(x)
#endif

#define NOT_YET_IMPLEMENTED()    throw std::logic_error(BUILD_STRING("not yet implemented: " << __FILE__ << ":" << __LINE__))
#define NOT_REACHED()            throw std::logic_error(BUILD_STRING("reached unreachable code: " << __FILE__ << ":" << __LINE__))
#define NOT_TESTED()             LOG_CRITICAL() << "not tested " << __FILE__ << " " << __LINE__

#define DECLARE_SHARED_POINTER_TYPES(Cls)          \
   typedef std::shared_ptr<Cls>  Cls ## Ptr;       \
   typedef std::weak_ptr<Cls>    Cls ## Ref;

#define NO_COPY_CONSTRUCTOR(Cls) \
   Cls(const Cls& other); \
   const Cls& operator=(const Cls& rhs);

#define VERIFY(cond, err) \
   do { \
      if (!(cond)) { \
         throw std::logic_error(err); \
      } \
   } while (false)

#endif // _RADIANT_MACROS_H
