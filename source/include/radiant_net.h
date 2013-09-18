#ifndef _RADIANT_NET_H
#define _RADIANT_NET_H

#include <libjson.h>

namespace radiant {
   namespace net {
      class IResponse
      {
      public:
         virtual ~IResponse() { }
         virtual void SetResponse(int status, std::string const& response, std::string const& mimeType) = 0;

         virtual void SetResponse(int status) {
            SetResponse(status, "", "");
         }
         virtual void SetJsonResponse(int status, JSONNode const& node) {
            SetResponse(status, node.write_formatted(), "application/json");
         }
         virtual void SetErrorResponse(std::string const& txt, std::exception &e) {
            JSONNode response;
            response.push_back(JSONNode("error", txt));
            response.push_back(JSONNode("reason", e.what()));
            SetResponse(500, response.write_formatted(), "application/json");
         }
      };
   };
};

#endif // _RADIANT_JSON_H
