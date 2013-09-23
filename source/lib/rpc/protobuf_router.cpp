#include "pch.h"
#include "radiant_stdutil.h"
#include "trace.h"
#include "untrace.h"
#include "function.h"
#include "protobuf_router.h"
#include "reactor_deferred.h"

using namespace ::radiant;
using namespace ::radiant::rpc;
namespace proto = radiant::tesseract::protocol;

ProtobufRouter::ProtobufRouter(SendRequestFn send_request_fn) :
   send_request_fn_(send_request_fn)
{
}

ReactorDeferredPtr ProtobufRouter::Call(Function const& fn)
{
   LOG(INFO) << "protobuf router dispatching call " << fn;

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
   LOG(INFO) << "protobuf router dispatching trace " << t;

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
   LOG(INFO) << "protobuf router creating deferred to deliver remote result";

   int call_id = r.call_id();
   ReactorDeferredPtr d = std::make_shared<ReactorDeferred>(std::string("remote " + stdutil::ToString(call_id) + " " + r.route()));
   pending_calls_[call_id] = d;

   d->Always([this, call_id]() {
      pending_calls_.erase(call_id);
   });
   send_request_fn_(r);

   return d;
}

ReactorDeferredPtr ProtobufRouter::RemoveTrace(UnTrace const& u)
{
   LOG(INFO) << "protobuf router removing trace " << u;
   for (const auto& entry : pending_calls_) {
      auto obj = entry.second.lock();
      if (obj) {
         LOG(INFO) << "  " << entry.first << " -> " << *obj;
      } else {
         LOG(INFO) << "  " << entry.first << " -> " << "expired";
      }
   }

   auto i = pending_calls_.find(u.call_id);
   if (i != pending_calls_.end()) {
      ReactorDeferredPtr d = i->second.lock();
      if (d) {
         d->ResolveWithMsg("trace removed");
      }
      pending_calls_.erase(i);
   } else {
      LOG(INFO) << "could not find trace!  that's... odd";
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
   auto i = pending_calls_.find(reply.call_id());
   if (i == pending_calls_.end()) {
      LOG(ERROR) << "received reply with unknown call id '" << reply.call_id() << "'";
      return;
   }

   JSONNode content;
   ReactorDeferredPtr d = i->second.lock();

   if (!d) {
      LOG(WARNING) << "deferred for pending call " << reply.call_id() << " is null.  removing.";
      // xxx: if this was a trace, we need to remove the trace from the remote end!
      pending_calls_.erase(i);
      return;
   }

   try {
      content = libjson::parse(reply.content());
   } catch (std::exception const& e) {
      LOG(ERROR) << "critical error parsing json content in protobuf router: " << e.what();
      LOG(ERROR) << "content was: " << reply.content();
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
