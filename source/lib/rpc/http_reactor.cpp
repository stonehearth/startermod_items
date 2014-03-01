#include "pch.h"
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "radiant_file.h"
#include "radiant_exceptions.h"
#include "http_reactor.h"
#include "http_deferred.h"
#include "resources/res_manager.h"
#include "core_reactor.h"
#include "reactor_deferred.h"
#include "function.h"
#include "trace.h"

using namespace ::radiant;
using namespace ::radiant::rpc;

HttpReactor::HttpReactor(CoreReactor &core) :
   core_(core),
   queued_events_(JSON_ARRAY)
{
   core_.AddRoute("radiant:get_events", [=](rpc::Function const& f) {
      return GetEvents(f);
   });
}

ReactorDeferredPtr HttpReactor::Call(json::Node const& query, std::string const& postdata)
{
   try {
      rpc::Function fn;
      fn.route = query.get<std::string>("fn", "");
      fn.object = query.get<std::string>("obj", "");

      RPC_LOG(5) << "http reactor dispatching " << fn;

      try {
         fn.args = libjson::parse(postdata);
      } catch (std::exception&) {
         throw core::InvalidArgumentException(BUILD_STRING("post data in http reactor is not valid json: " << postdata));
      }
      ReactorDeferredPtr d = core_.Call(fn);
      if (!d->IsPending() || query.get<int>("long_poll", 0) != 0) {
         // go ahead and send the result now as a result of the GET.
         return d;
      } 
      return CreateDeferredResponse(fn, d);


   } catch (std::exception &e) {
      RPC_LOG(3) << "critical error in http reactor: " << e.what();
   }
   return nullptr;
}

ReactorDeferredPtr HttpReactor::InstallTrace(Trace const& t)
{
   int code;
   std::string content, mimetype;
   if (boost::ends_with(t.route, ".json") && HttpGetFile(t.route, code, content, mimetype)) {
      // if we wanted to, we could put a file system watcher on this path now
      // and call Notify again later. =)
      ReactorDeferredPtr d = std::make_shared<ReactorDeferred>(BUILD_STRING("http response to " << t));
      try {
         d->Notify(libjson::parse(content));
      } catch (std::exception& e) {
         RPC_LOG(3) << "error trying to trace filesystem .json file: " << e.what();
         return nullptr;
      }
      return d;
   }

   return core_.InstallTrace(t);
}

ReactorDeferredPtr HttpReactor::CreateDeferredResponse(Function const& fn, ReactorDeferredPtr d)
{
   RPC_LOG(5) << "http reactor creating deferred response for " << *d;

   // send the result later in the event queue
   int call_id = fn.call_id;
   auto queue_event = [this, call_id](const char* t, JSONNode data) {
      JSONNode node;
      node.push_back(JSONNode("call_id", call_id));
      data.set_name("data");
      node.push_back(data);
      RPC_LOG(5) << "http reactor queuing " << t << " event for call_id " << call_id << ".";
      QueueEvent(t, node);
   };
   d->Progress([d, queue_event](JSONNode node) {
      queue_event("radiant_call_progress", node);
   });
   d->Done([d, queue_event](JSONNode node) {
      queue_event("radiant_call_done", node);
   });
   d->Fail([d, queue_event](JSONNode node) {
      queue_event("radiant_call_fail", node);
   });

   // return a deferred deferred =)
   RPC_LOG(5) << "http reactor creating deferred to deliver call_deferred result";
   ReactorDeferredPtr result = std::make_shared<ReactorDeferred>(std::string("http response to ") + fn.desc());
   JSONNode done;
   done.push_back(JSONNode("type",     "radiant_call_deferred"));
   done.push_back(JSONNode("call_id",  call_id));
   result->Resolve(done);

   return result;
}

ReactorDeferredPtr HttpReactor::GetEvents(rpc::Function const& f)
{
   if (get_events_deferred_) {
      get_events_deferred_->RejectWithMsg("called get_events while existing request is pending");
   }
   ReactorDeferredPtr d = std::make_shared<ReactorDeferred>(f.desc());
   get_events_deferred_ = d;
   FlushEvents();
   return d;
}

void HttpReactor::FlushEvents()
{
   if (get_events_deferred_ && !queued_events_.empty()) {
      RPC_LOG(5) << "flushing http_reactor events.";
      get_events_deferred_->Resolve(queued_events_);
      get_events_deferred_ = nullptr;
      queued_events_ = JSONNode(JSON_ARRAY);
   }
}

void HttpReactor::QueueEvent(std::string type, JSONNode payload)
{
   RPC_LOG(5) << "queuing new event of type " << type << " in http reactor.";

   JSONNode e(JSON_NODE);
   e.push_back(JSONNode("type", type));

   payload.set_name("data");
   e.push_back(payload);

   queued_events_.push_back(e);
}

// must be thread safe.  called from the client browser thread
bool HttpReactor::HttpGetFile(std::string const& uri, int &code, std::string& content, std::string& mimetype)
{
   // xxx: this is in no way thread safe! (see SH-8)
   static const struct {
      char *extension;
      char *mimeType;
   } mimeTypes_[] = {
      { "htm",  "text/html" },
      { "html", "text/html" },
      {  "css",  "text/css" },
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
   auto const& rm = res::ResourceManager2::GetInstance();

   try {
      if (boost::ends_with(uri, ".json")) {
         rm.LookupJson(uri, [&](const JSONNode& node) {
            content = node.write();
         });
      } else {
         RPC_LOG(5) << "reading file " << uri;
         std::shared_ptr<std::istream> is = rm.OpenResource(uri);
         content = io::read_contents(*is);
      }
   } catch (std::exception& e) {
      RPC_LOG(3) << "error code 404: " << e.what();
      code = 404;
      content = e.what();
      return false;
   }

   code = 200;

   // Determine the file extension.
   std::string mimeType;
   std::size_t last_dot_pos = uri.find_last_of(".");
   if (last_dot_pos != std::string::npos) {
      std::string extension = uri.substr(last_dot_pos + 1);
      for (auto &entry : mimeTypes_) {
         if (extension == entry.extension) {
            mimetype = entry.mimeType;
            break;
         }
      }
   }
   if (mimetype.empty()) {
      RPC_LOG(3) << "unrecognized mime type for uri: " <<  uri;
   }
   return true;
}