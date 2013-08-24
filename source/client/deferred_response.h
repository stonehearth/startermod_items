#ifndef _RADIANT_CLIENT_DEFERRED_RESPONSE_H
#define _RADIANT_CLIENT_DEFERRED_RESPONSE_H

#include "radiant_net.h"
#include "radiant_json.h"
#include "core/deferred.h"
#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class DeferredResponse :  public net::IResponse,
                          public core::Deferred1<JSONNode>
{
public: // the response...
   void SetResponse(int code) override {
      SetResponse(code, "", "");
   }
   void SetResponse(int code, std::string const& data, std::string const& mimetype) override {
      JSONNode result;
      if (!data.empty()) {
         result = libjson::parse(data);
      }
      if (code == 200) {
         Resolve(result);
      } else {
         Reject(result);
      }
   }
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_DEFERRED_RESPONSE_H
