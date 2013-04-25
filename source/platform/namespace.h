#ifndef _RADIANT_PLATFORM_NAMESPACE_H
#define _RADIANT_PLATFORM_NAMESPACE_H

#define BEGIN_RADIANT_PLATFORM_NAMESPACE  namespace radiant { namespace platform {
#define END_RADIANT_PLATFORM_NAMESPACE    } }

#define RADIANT_PLATFORM_NAMESPACE    ::radiant::platform

#define IN_RADIANT_PLATFORM_NAMESPACE(x)      \
   BEGIN_RADIANT_PLATFORM_NAMESPACE        \
   x                                    \
   END_RADIANT_PLATFORM_NAMESPACE

#endif //  _RADIANT_PLATFORM_NAMESPACE_H
