#include "pch.h"
#include "response.h"
#include <fstream>

using namespace radiant;
using namespace radiant::chromium;

Response::Response(CefRefPtr<CefRequest> request) :
   request_(request),
   status_(0),
   readOffset_(0),
   mimeType_("text/html")
{
   refCount_ = 0;
}

Response::~Response()
{
}

int Response::AddRef()
{
   return ++refCount_;
}

int Response::Release() 
{
   if (--refCount_ == 0) {
      delete this;
   }
   return refCount_;
}

int Response::GetRefCt()
{ 
   return refCount_; 
}

// Begin processing the request. To handle the request return true and call
// HeadersAvailable() once the response header information is available
// (HeadersAvailable() can also be called from inside this method if header
// information is available immediately). To cancel the request return false.

bool Response::ProcessRequest(CefRefPtr<CefRequest> request,
                              CefRefPtr<CefCallback> callback)
{
   callback_ = callback;
   if (status_ != 0) {
      callback->Continue();
   }
   return true;
}

void Response::GetResponseHeaders(CefRefPtr<CefResponse> response,
                                  int64& response_length,
                                  CefString& redirectUrl)
{
   if (!mimeType_.empty()) {
      response->SetMimeType(mimeType_);
   }
   response->SetStatus(status_);
   response_length = response_.size();
}

bool Response::ReadResponse(void* data_out,
                            int bytes_to_read,
                            int& bytes_read,
                            CefRefPtr<CefCallback> callback)
{
   if (readOffset_ >= response_.size()) {
      return false;
   }
   bytes_read = std::min((unsigned int)bytes_to_read, response_.size() - readOffset_);
   memcpy(data_out, response_.c_str() + readOffset_, bytes_read);
   readOffset_ += bytes_read;
   return true;
}

void Response::Cancel()
{
}

void Response::SetResponse(int status_code, std::string const& response, std::string const& mimeType)
{
   ASSERT(status_ == 0);

   status_ = status_code;
   mimeType_ = mimeType;
   response_ = response;

   if (callback_) {
      callback_->Continue();
   }
}

#if 0
void Response::SetResponse(JSONNode node)
{
   SetResponse(node.write(), "application/json");
}
#endif
