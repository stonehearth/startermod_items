#include "pch.h"
#include "radiant_exceptions.h"
#include "protobuf_reactor.h"
#include "reactor_deferred.h"
#include "core_reactor.h"
#include "trace.h"
#include "untrace.h"
#include "function.h"
#include "session.h"
#include <snappy.h>

using namespace ::radiant;
using namespace ::radiant::rpc;
namespace proto = radiant::tesseract::protocol;

ProtobufReactor::ProtobufReactor(CoreReactor &core, SendReplyFn send_reply_fn) :
   core_(core),
   send_reply_fn_(send_reply_fn)
{
}

ReactorDeferredPtr ProtobufReactor::Dispatch(SessionPtr session, proto::PostCommandRequest const& msg)
{
   int call_id = msg.call_id();

   RPC_LOG(5) << "protobuf reactor dispatching " << msg.route() << "(call_id:" << call_id << ")";

   auto send_reply = [call_id, this](proto::PostCommandReply_Type type, JSONNode const& node) {
      RPC_LOG(5) << "protobuf reactor got reply  (call_id:" << call_id << " type:" << type << ")";
      proto::PostCommandReply reply;
      reply.set_call_id(call_id);
      reply.set_type(type);

      // these can get really big, so snappy compress them.
      std::string json = node.write();
      snappy::Compress(json.c_str(), json.size(), reply.mutable_content());

      send_reply_fn_(reply);
   };

   try {
      ReactorDeferredPtr d;
      if (msg.op() == msg.CALL) {
         JSONNode args = libjson::parse(msg.args());
         try {
            args = libjson::parse(msg.args());
         } catch (std::exception &) {
            throw core::InvalidArgumentException(BUILD_STRING("args in protobuf reactor are not valid json: " << msg.args()));
         }
         rpc::Function fn(session, call_id, msg.route(), args);
         fn.object = msg.object();
         d = core_.Call(fn);
      } else if (msg.op() == msg.INSTALL_TRACE) {
         d = core_.InstallTrace(rpc::Trace(session, call_id, msg.route()));
         traces_[call_id] = d;
      } else if (msg.op() == msg.REMOVE_TRACE) {
         d = core_.RemoveTrace(rpc::UnTrace(session, call_id));
         traces_.erase(call_id);
      }
      if (d) {
         d->Progress([send_reply](JSONNode node) {
            send_reply(proto::PostCommandReply::PROGRESS, node);
         });
         d->Done([send_reply](JSONNode node) {
            send_reply(proto::PostCommandReply::DONE, node);
         });
         d->Fail([send_reply](JSONNode node) {
            send_reply(proto::PostCommandReply::FAIL, node);
         });
      }

      return d;
   } catch (std::exception &e) {
      JSONNode fail;
      fail.push_back(JSONNode("error", e.what()));
      send_reply(proto::PostCommandReply::FAIL, fail);
   }
   return nullptr;
}
