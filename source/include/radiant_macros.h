#ifndef _RADIANT_MACROS_H
#define _RADIANT_MACROS_H

#define OFFSET_OF(t, f)    (int)(&((t *)(NULL))->f)
#define ARRAY_SIZE(a)      ((sizeof (a)) / sizeof((a)[0]))

#if defined(ASSERT)
#  error ASSERT defined before inclusion of radiant_macros.h.  Aborting
#endif

#define ASSERT(x) \
   do { /* xxx: relegate all this logic to some Assert class */ \
      if (!(x)) { \
         if (IsDebuggerPresent()) { \
            DebugBreak(); \
         } else { \
            ::MessageBox(NULL, #x, "Stonehearth Assertion Failed", MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL); \
         } \
      } \
   } while(false)

#define NYI_ERROR_CHECK(x) ASSERT(x)

#define BUILD_STRING(x) static_cast<std::ostringstream&>(std::ostringstream() << x).str()

#define DEBUG_ONLY(x)      do { x } while (false);
#define NOT_YET_IMPLEMENTED()       throw std::logic_error(BUILD_STRING("not yet implemented: " << __FILE__ << ":" << __LINE__))
#define NOT_REACHED()      ASSERT(false)

#define DECLARE_SHARED_POINTER_TYPES(Cls) \
   typedef std::shared_ptr<Cls>  Cls ## Ptr; \
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
