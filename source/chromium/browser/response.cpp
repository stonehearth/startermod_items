#include "radiant.h"
#include "chromium/cef_headers.h"
#include "response.h"
#include <fstream>

using namespace radiant;
using namespace radiant::chromium;

Response::Response(CefRefPtr<CefRequest> request) :
   _request(request),
   _status(0),
   _readOffset(0),
   _mimeType("text/html")
{
   _refCount = 0;
}

Response::~Response()
{
}

int Response::AddRef()
{
   return ++_refCount;
}

int Response::Release() 
{
   if (--_refCount == 0) {
      delete this;
   }
   return _refCount;
}

int Response::GetRefCt()
{ 
   return _refCount; 
}

// Begin processing the request. To handle the request return true and call
// HeadersAvailable() once the response header information is available
// (HeadersAvailable() can also be called from inside this method if header
// information is available immediately). To cancel the request return false.

bool Response::ProcessRequest(CefRefPtr<CefRequest> request,
                              CefRefPtr<CefCallback> callback)
{
   _callback = callback;
   if (_status != 0) {
      callback->Continue();
   }
   return true;
}

void Response::GetResponseHeaders(CefRefPtr<CefResponse> response,
                                  ::int64& response_length,
                                  CefString& redirectUrl)
{
   if (!_mimeType.empty()) {
      response->SetMimeType(_mimeType);
   }
   response->SetStatus(_status);
   response_length = _response.size();
}

bool Response::ReadResponse(void* data_out,
                            int bytes_to_read,
                            int& bytes_read,
                            CefRefPtr<CefCallback> callback)
{
   if (_readOffset >= _response.size()) {
      return false;
   }
   bytes_read = std::min((unsigned int)bytes_to_read, _response.size() - _readOffset);
   memcpy(data_out, _response.c_str() + _readOffset, bytes_read);
   _readOffset += bytes_read;
   return true;
}

void Response::Cancel()
{
}

void Response::SetResponse(int status_code, std::string const& response, std::string const& mimeType)
{
   ASSERT(_status == 0);

   _status = status_code;
   _mimeType = mimeType;
   _response = response;

   if (_callback) {
      _callback->Continue();
   }
}
