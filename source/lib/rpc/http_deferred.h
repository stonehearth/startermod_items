#ifndef _RADIANT_LIB_RPC_HTTP_DEFERRED_H
  #define _RADIANT_LIB_RPC_HTTP_DEFERRED_H

#include "namespace.h"
#include "core/deferred.h"

BEGIN_RADIANT_RPC_NAMESPACE

struct HttpResponse {
   int code;
   std::string content;
   std::string mime_type;
   std::unordered_map<std::string, std::string> headers;

   HttpResponse() : code(500), content("uninitialized response"), mime_type("plain/text") { }
   HttpResponse(int c, std::string const& r, std::string const& m) : code(c), content(r), mime_type(m) { }
   HttpResponse(int c, std::string const& r, std::string const& m, std::unordered_map<std::string, std::string> const& h) : code(c), content(r), mime_type(m), headers(h) { }
};

struct HttpError {
   int code;
   std::string response;

   HttpError() : code(500), response("uninitialized error") { }
   HttpError(int c, std::string const& r) : code(c), response(r) { }
};

class HttpDeferred : public core::Deferred<HttpResponse, HttpError>
{
public:
   HttpDeferred(std::string const& dbg_name) : core::Deferred<HttpResponse, HttpError>(dbg_name) {}

   void AddHeader(const char* name, const char* value);
   void RejectWithError(int code, const char* reason);
   void ResolveWithFile(std::string const& path);
   void ResolveWithContent(std::string const& content, std::string const& mimetype);

private:
   std::unordered_map<std::string, std::string>    _headers;
};


DECLARE_SHARED_POINTER_TYPES(HttpDeferred)

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_HTTP_DEFERRED_H
