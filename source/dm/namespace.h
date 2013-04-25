#ifndef _RADIANT_DM_NAMESPACE_H
#define _RADIANT_DM_NAMESPACE_H

#define BEGIN_RADIANT_DM_NAMESPACE  namespace radiant { namespace dm {
#define END_RADIANT_DM_NAMESPACE    } }

#define RADIANT_DM_NAMESPACE ::radiant::dm

#define IN_RADIANT_DM_NAMESPACE(x)      \
   BEGIN_RADIANT_DM_NAMESPACE        \
   x                                    \
   END_RADIANT_DM_NAMESPACE

#include "types.h"

#endif //  _RADIANT_DM_NAMESPACE_H
