#ifndef _RADIANT_CLIENT_DEFERRED_H
#define _RADIANT_CLIENT_DEFERRED_H

#include "radiant_net.h"
#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Deferred : public net::IResponse
{
public:
   Deferred();
   ~Deferred();
   
   void Always(std::function<void()> cb);

public:
   void SetResponse(int code) override;
   void SetResponse(int code, std::string const& data, std::string const& mimetype) override;

private:
   void Resolve();

private:
   int                                code_;
   std::string                        data_;
   std::string                        mimeType_;
   std::vector<std::function<void()>> alwaysCbs_;
};

typedef std::shared_ptr<Deferred> DeferredPtr;
typedef std::weak_ptr<Deferred> DeferredRef;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_DEFERRED_H
