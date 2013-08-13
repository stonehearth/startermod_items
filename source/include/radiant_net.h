#ifndef _RADIANT_NET_H
#define _RADIANT_NET_H

namespace radiant {
   namespace net {
      class IResponse
      {
      public:
         virtual ~IResponse() { }
         virtual void SetResponse(int status) { SetResponse(status, "", ""); }
         virtual void SetResponse(int status, std::string const& response, std::string const& mimeType) = 0;
      };
   };
};

#endif // _RADIANT_JSON_H
