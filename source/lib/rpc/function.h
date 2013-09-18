#ifndef _RADIANT_LIB_RPC_REACTOR_FUNCTION_H
#define _RADIANT_LIB_RPC_REACTOR_FUNCTION_H

#include "request.h"
#include "radiant_json.h"

BEGIN_RADIANT_RPC_NAMESPACE

class Function : public Request 
{
public:
   Function() :
      args(JSONNode())
   {
   }

   Function(std::string const& r, json::ConstJsonObject const& a) :
      route(r),
      args(a)
   {
   }

   Function(SessionPtr s, int id, std::string const& r, json::ConstJsonObject const& a) :
      Request(s, id),
      route(r),
      args(a)
   {
   }

   std::string desc() const;

public:
   std::string                object;
   std::string                route;
   json::ConstJsonObject      args;
};

static inline std::ostream& operator<<(std::ostream& os, Function const& f) {
   return (os << f.desc());
}

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_REACTOR_FUNCTION_H
