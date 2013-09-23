#ifndef _RADIANT_LIB_RPC_REACTOR_UNTRACE_H
#define _RADIANT_LIB_RPC_REACTOR_UNTRACE_H

#include "request.h"

BEGIN_RADIANT_RPC_NAMESPACE

class UnTrace : public Request
{
public:
   UnTrace(SessionPtr s, int call_id) :
      Request(s, call_id)
   {
   }

public:
   std::string desc() const;
};

static inline std::ostream& operator<<(std::ostream& os, UnTrace const& f) {
   return (os << f.desc());
}

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_REACTOR_TRACE_H
