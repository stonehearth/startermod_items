#ifndef _RADIANT_TYPES_H
#define _RADIANT_TYPES_H

namespace radiant {
   typedef unsigned char      uchar;

   typedef unsigned int       uint, uint32, uint;
   typedef signed int         sint32, int32;

   typedef unsigned short     uint16;
   typedef signed short       sint16, int16;

   // conflicts with cef_types.h ... why don't they have type guards!!
   // typedef signed __int64     sint64, int64;
   // typedef unsigned __int64   uint64;


   // helper functions for types

   template <class T>
   static inline const char* GetShortTypeName()
   {
      const char* name = typeid(T).name();
      const char* last = strrchr(name, ':');
      return last ? last + 1 : name;
   }
};

#endif // _RADIANT_TYPES_H
