#ifndef _RADIANT_LIB_RPC_REACTOR_SESSION_H
#define _RADIANT_LIB_RPC_REACTOR_SESSION_H

#include "request.h"
#include "lib/rpc/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class Session 
{
public:

public:
   std::string          faction;
};

std::ostream& operator<<(std::ostream& os, Session const& s);

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_REACTOR_SESSION_H
