#ifndef _RADIANT_CLIENT_RESPONSE_H
#define _RADIANT_CLIENT_RESPONSE_H

#include "namespace.h"
#include "libjson.h"
#include <atomic>

BEGIN_RADIANT_CLIENT_NAMESPACE

typedef int ResponseId;
typedef std::function<void(std::string const& )> ResponseFn;

class Response
{
public:
   Response(ResponseFn r);
   ~Response();

   int GetSession() const;
   void SetSession(int session);

   void Complete(std::string const& result);
   void Defer(int id);
   void Error(std::string const& reason);

private:
   static std::atomic_int nextDeferId_;

private:
   int            session_;
   ResponseFn     responseFn_;
};

typedef std::shared_ptr<Response> ResponsePtr;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RESPONSE_H
