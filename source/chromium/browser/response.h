#ifndef _RADIANT_CHROMIUM_BROWSER_RESPONSE_H
#define _RADIANT_CHROMIUM_BROWSER_RESPONSE_H

#include <atomic>
#include "../chromium.h"

BEGIN_RADIANT_CHROMIUM_NAMESPACE

class Response : public IResponse,
                 public CefResourceHandler
{
public:
   Response(CefRefPtr<CefRequest> request);
   virtual ~Response();

   void SetResponse(int status, std::string const& response, std::string const& mimeType) override;

public:
   // CefBase overrides
   int AddRef() override;
   int Release() override;
   int GetRefCt() override;

   // CefSchemeHandler overrides

   //
   // Begin processing the request. To handle the request return true and call
   // CefCallback::Continue() once the response header information is available
   // (CefCallback::Continue() can also be called from inside this method if
   // header information is available immediately). To cancel the request return
   // false.
   //
   bool ProcessRequest(CefRefPtr<CefRequest> request,
                       CefRefPtr<CefCallback> callback) override;

   ///
   // Retrieve response header information. If the response length is not known
   // set |response_length| to -1 and ReadResponse() will be called until it
   // returns false. If the response length is known set |response_length|
   // to a positive value and ReadResponse() will be called until it returns
   // false or the specified number of bytes have been read. Use the |response|
   // object to set the mime type, http status code and other optional header
   // values. To redirect the request to a new URL set |redirectUrl| to the new
   // URL.
   ///
   void GetResponseHeaders(CefRefPtr<CefResponse> response,
                           int64& response_length,
                           CefString& redirectUrl) override;

   ///
   // Read response data. If data is available immediately copy up to
   // |bytes_to_read| bytes into |data_out|, set |bytes_read| to the number of
   // bytes copied, and return true. To read the data at a later time set
   // |bytes_read| to 0, return true and call CefCallback::Continue() when the
   // data is available. To indicate response completion return false.
   ///
   bool ReadResponse(void* data_out,
                     int bytes_to_read,
                     int& bytes_read,
                     CefRefPtr<CefCallback> callback) override;

   ///
   // Request processing has been canceled.
   ///
   void Cancel() override;

private:
   std::atomic_int               refCount_;
   CefRefPtr<CefRequest>         request_;
   CefRefPtr<CefCallback>        callback_;
   std::string                   response_;
   unsigned int                  readOffset_;
   int                           status_;
   std::string                   mimeType_;
};

END_RADIANT_CHROMIUM_NAMESPACE

#endif // _RADIANT_CHROMIUM_BROWSER_RESPONSE_H
