#include "pch.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "trace.h"
#include "untrace.h"
#include "function.h"
#include "protobuf_router.h"
#include "reactor_deferred.h"
#include <snappy.h>

using namespace ::radiant;
using namespace ::radiant::rpc;
namespace proto = radiant::tesseract::protocol;

ProtobufRouter::ProtobufRouter(SendRequestFn send_request_fn) :
   send_request_fn_(send_request_fn)
{
}

ReactorDeferredPtr ProtobufRouter::Call(Function const& fn)
{
   RPC_LOG(5) << "protobuf router dispatching call " << fn << "(call_id:" << fn.call_id << ")";

   proto::PostCommandRequest request;
   request.set_op(request.CALL);
   request.set_object(fn.object);
   request.set_route(fn.route);
   request.set_args(fn.args.write());
   request.set_call_id(fn.call_id);

   return SendNewRequest(request);
}

ReactorDeferredPtr ProtobufRouter::InstallTrace(Trace const& t)
{
   RPC_LOG(5) << "protobuf router dispatching trace " << t << "(call_id:" << t.call_id << ")";

   // xxx: if multiple clients all trace the same uri, we should only trace the
   // remote end once.

   proto::PostCommandRequest request;
   request.set_op(request.INSTALL_TRACE);
   request.set_route(t.route);
   request.set_call_id(t.call_id);

   int call_id = t.call_id;
   ReactorDeferredPtr trace = SendNewRequest(request);
   trace->Always([call_id, this]() {
      DestroyRemoteTrace(call_id);
   });

   return trace;
}

ReactorDeferredPtr ProtobufRouter::SendNewRequest(proto::PostCommandRequest& r)
{
   int call_id = r.call_id();

   RPC_LOG(5) << "protobuf router creating deferred to deliver remote result " << "(call_id:" << call_id << ")";

   ReactorDeferredPtr d = std::make_shared<ReactorDeferred>(std::string("remote " + stdutil::ToString(call_id) + " " + r.route()));
   pending_calls_[call_id] = d;

   d->Always([this, call_id]() {
      RPC_LOG(5) << "call finished.  erasing from pending call map (call_id: " << call_id << ")";
      pending_calls_.erase(call_id);
   });
   send_request_fn_(r);

   return d;
}

ReactorDeferredPtr ProtobufRouter::RemoveTrace(UnTrace const& u)
{
   RPC_LOG(5) << "protobuf router removing trace "  << "(call_id:" << u.call_id << ")";

   auto i = pending_calls_.find(u.call_id);
   if (i != pending_calls_.end()) {
      i->second->ResolveWithMsg("trace removed");
      pending_calls_.erase(i);
   } else {
      RPC_LOG(5) << "could not find trace!  that's... odd";
   }

   ReactorDeferredPtr d = std::make_shared<ReactorDeferred>("remove trace");
   d->ResolveWithMsg(BUILD_STRING("trace " << u.call_id << " removed"));
   return d;
}

void ProtobufRouter::DestroyRemoteTrace(int call_id)
{
   proto::PostCommandRequest request;
   request.set_op(request.REMOVE_TRACE);
   request.set_call_id(call_id);
   send_request_fn_(request);
}

void ProtobufRouter::OnPostCommandReply(proto::PostCommandReply const& reply)
{
   int call_id = reply.call_id();
   RPC_LOG(5) << "protobuf router received reply (call_id:" << call_id << ")";

   auto i = pending_calls_.find(call_id);
   if (i == pending_calls_.end()) {
      RPC_LOG(1) << "received reply with unknown call id '" << call_id << "'";
      return;
   }

   JSONNode content;
   ReactorDeferredPtr d = i->second;

   std::string json;
   std::string const& compressed_content = reply.content();
   if (!snappy::Uncompress(compressed_content.c_str(), compressed_content.size(), &json)) {
      RPC_LOG(1) << "could not decompress command reply content buffer"; 
      d->RejectWithMsg("could not decompress command reply content buffer");
      pending_calls_.erase(i);
      return;
   }

   try {
      content = libjson::parse(json);
   } catch (std::exception const& e) {
      RPC_LOG(1) << "critical error parsing json content in protobuf router: " << e.what();
      RPC_LOG(1) << "content was: " << reply.content();
      d->RejectWithException(e);
      pending_calls_.erase(i);
      return;
   }

   switch (reply.type()) {
      case proto::PostCommandReply::DONE:
         d->Resolve(content);
         break;
      case proto::PostCommandReply::PROGRESS:
         d->Notify(content);
         break;
      case proto::PostCommandReply::FAIL:
         d->Reject(content);
         break;
   }
}
