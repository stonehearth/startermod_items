#ifndef _RADIANT_LIB_RPC_REACTOR_TRACE_H
#define _RADIANT_LIB_RPC_REACTOR_TRACE_H

#include "request.h"

BEGIN_RADIANT_RPC_NAMESPACE

class Trace : public Request
{
public:
   Trace(std::string const& r) :
      route(r)
   {
   }

   Trace(SessionPtr s, int id, std::string const& r) :
      Request(s, id),
      route(r)
   {
   }

   std::string desc() const;

public:
   SessionPtr     caller;
   std::string    route;
   std::string    reason;
};

static inline std::ostream& operator<<(std::ostream& os, Trace const& f) {
   return (os << f.desc());
}

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_REACTOR_TRACE_H
