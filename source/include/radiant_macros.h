#ifndef _RADIANT_MACROS_H
#define _RADIANT_MACROS_H

#define OFFSET_OF(t, f)    (int)(&((t *)(NULL))->f)
#define ARRAY_SIZE(a)      ((sizeof (a)) / sizeof((a)[0]))

#if defined(ASSERT)
// xxx: find them and kill them!
#undef ASSERT
#endif

#define ASSERT(x)          do { if (!(x)) { DebugBreak(); } } while(false)
#define NYI_ERROR_CHECK(x) ASSERT(x)

#define VERIFY(expr, Excp) do { if (!(expr)) { throw Excp; } } while(false)

#define DEBUG_ONLY(x)      do { x } while (false);
#define RADIANT_NOT_IMPLEMENTED()   (LOG(WARNING) << __FUNCTION__  << " not implemented (" << __FILE__ << ":" << __LINE__)
#define NOT_YET_IMPLEMENTED()       (LOG(WARNING) << __FUNCTION__  << " not implemented (" << __FILE__ << ":" << __LINE__)
#define NOT_REACHED()      ASSERT(false)

#define BUILD_STRING(x) static_cast<std::ostringstream&>(std::ostringstream() << x).str()

#define DECLARE_SHARED_POINTER_TYPES(Cls) \
   typedef std::shared_ptr<Cls>  Cls ## Ptr; \
   typedef std::weak_ptr<Cls>    Cls ## Ref;

#define NO_COPY_CONSTRUCTOR(Cls) \
   Cls(const Cls& other); \
   const Cls& operator=(const Cls& rhs);

#endif // _RADIANT_MACROS_H
