#include "pch.h"
#include <fstream>
#include "radiant_file.h"
#include "http_deferred.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

void HttpDeferred::RejectWithError(int code, const char* reason)
{
   Reject(HttpError(code, reason));
}

void HttpDeferred::ResolveWithContent(std::string const& content, std::string const& mimetype)
{
   Resolve(HttpResponse(200, content, mimetype));
}

void HttpDeferred::ResolveWithFile(std::string const& filename)
{
   static const struct {
      char *extension;
      char *mimeType;
   } mimeTypes_[] = {
      { "htm",  "text/html" },
      { "html", "text/html" },
      { "css",  "text/css" },
      { "less",  "text/css" },
      { "js",   "application/x-javascript" },
      { "json", "application/json" },
      { "txt",  "text/plain" },
      { "jpg",  "image/jpeg" },
      { "png",  "image/png" },
      { "gif",  "image/gif" },
      { "woff", "application/font-woff" },
      { "cur",  "image/vnd.microsoft.icon" },
   };

   std::string content;
   try {
      RPC_LOG(5) << "reading file " << filename;
      std::ifstream is(filename, std::ios::in | std::ios::binary);
      content = io::read_contents(is);
   } catch (std::exception& e) {
      RejectWithError(404, e.what());
      return;
   }
   std::string mimetype;

   // Determine the file extension.
   std::size_t last_dot_pos = filename.find_last_of(".");
   if (last_dot_pos != std::string::npos) {
      std::string extension = filename.substr(last_dot_pos + 1);
      for (auto &entry : mimeTypes_) {
         if (extension == entry.extension) {
            mimetype = entry.mimeType;
            break;
         }
      }
   }   
   ResolveWithContent(content, mimetype);
}
