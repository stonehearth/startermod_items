#ifndef _RADIANT_OM_NAMESPACE_H
#define _RADIANT_OM_NAMESPACE_H

#define BEGIN_RADIANT_OM_NAMESPACE  namespace radiant { namespace om {
#define END_RADIANT_OM_NAMESPACE    } }

#define RADIANT_OM_NAMESPACE ::radiant::om

#define IN_RADIANT_OM_NAMESPACE(x)      \
   BEGIN_RADIANT_OM_NAMESPACE        \
   x                                    \
   END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_NAMESPACE_H
