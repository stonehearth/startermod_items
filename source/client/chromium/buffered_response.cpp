#include "pch.h"
#include "buffered_response.h"
#include <fstream>

using namespace radiant;
using namespace radiant::client;

BufferedResponse::BufferedResponse(CefRefPtr<CefRequest> request) :
   refCount_(0),
   request_(request),
   status_(0),
   readOffset_(0),
   mimeType_("text/html")
{
}

BufferedResponse::~BufferedResponse()
{
}

int BufferedResponse::AddRef()
{
   return ++refCount_;
}

int BufferedResponse::Release() 
{
   if (--refCount_ == 0) {
      delete this;
   }
   return refCount_;
}

int BufferedResponse::GetRefCt()
{ 
   return refCount_; 
}

// Begin processing the request. To handle the request return true and call
// HeadersAvailable() once the response header information is available
// (HeadersAvailable() can also be called from inside this method if header
// information is available immediately). To cancel the request return false.

bool BufferedResponse::ProcessRequest(CefRefPtr<CefRequest> request,
                                     CefRefPtr<CefCallback> callback)
{
   callback_ = callback;
   if (status_ != 0) {
      callback->Continue();
   }
   return true;
}

void BufferedResponse::GetResponseHeaders(CefRefPtr<CefResponse> response,
                                    int64& response_length,
                                    CefString& redirectUrl)
{
   response->SetMimeType(mimeType_);
   response->SetStatus(status_);
   response_length = response_.size();
}

bool BufferedResponse::ReadResponse(void* data_out,
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

void BufferedResponse::Cancel()
{
}

void BufferedResponse::SetStatusCode(int code)
{
   ASSERT(status_ == 0 && code != 0);

   status_ = code;
   if (callback_) {
      callback_->Continue();
   }
}

void BufferedResponse::SetResponse(std::string response, std::string mimeType)
{
   ASSERT(status_ == 0);

   status_ = 200;
   mimeType_ = mimeType;
   response_ = response;

   if (callback_) {
      callback_->Continue();
   }
}

void BufferedResponse::SetResponse(JSONNode node)
{
   SetResponse(node.write(), "application/json");
}
