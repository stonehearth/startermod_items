#ifndef _RADIANT_RESOURCES_NAMESPACE_H
#define _RADIANT_RESOURCES_NAMESPACE_H

#define BEGIN_RADIANT_RESOURCES_NAMESPACE  namespace radiant { namespace resources {
#define END_RADIANT_RESOURCES_NAMESPACE    } }

#define RADIANT_RESOURCES_NAMESPACE    ::radiant::resources

#define IN_RADIANT_RESOURCES_NAMESPACE(x) \
   BEGIN_RADIANT_RESOURCES_NAMESPACE \
   x  \
   END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_NAMESPACE_H
