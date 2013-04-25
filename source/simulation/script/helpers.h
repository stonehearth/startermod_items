#ifndef _RADIANT_SIMULATION_SCRIPT_HELPERS_H
#define _RADIANT_SIMULATION_SCRIPT_HELPERS_H

#include "namespace.h"

BEGIN_RADIANT_SIMULATION_NAMESPACE

// xxx - Once we have variadic template support we'll be able to consolidate all this (sigh).



END_RADIANT_SIMULATION_NAMESPACE


template <class T>
static std::string DefaultToWatch(const T& obj)
{
   std::ostringstream out;
   if (&obj) {
      out << obj;
   } else {
      out << "nullptr";
   }
   return out.str();
}

#endif //  _RADIANT_SIMULATION_SCRIPT_HELPERS_H