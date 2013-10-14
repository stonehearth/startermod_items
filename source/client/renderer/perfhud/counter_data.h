#ifndef _RADIANT_CLIENT_RENDERER_COUNTER_DATA_H
#define _RADIANT_CLIENT_RENDERER_COUNTER_DATA_H

#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class CounterData {
public:
   CounterData() { }
   CounterData(std::string const& n ) : name(n) { }

   std::string    name;
   csg::Color3    color;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_COUNTER_DATA_H