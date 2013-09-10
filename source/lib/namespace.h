#ifndef _RADIANT_LIB_NAMESPACE_H
#define _RADIANT_LIB_NAMESPACE_H

#define BEGIN_RADIANT_LIB_NAMESPACE  namespace radiant { namespace lib {
#define END_RADIANT_LIB_NAMESPACE    } }

#define RADIANT_LIB_NAMESPACE    ::radiant::lib

#define IN_RADIANT_LIB_NAMESPACE(x) \
   BEGIN_RADIANT_LIB_NAMESPACE \
   x  \
   END_RADIANT_LIB_NAMESPACE

#endif //  _RADIANT_LIB_NAMESPACE_H
