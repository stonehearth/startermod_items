#ifndef _RADIANT_LIB_RPC_PROTOBUF_ROUTER_H
#define _RADIANT_LIB_RPC_PROTOBUF_ROUTER_H

#include <unordered_map>
#include "irouter.h"
#include "forward_defines.h"
#include "protocols/tesseract.pb.h"

BEGIN_RADIANT_RPC_NAMESPACE

class ProtobufRouter : public IRouter {
public:
   typedef std::function<void(radiant::tesseract::protocol::PostCommandRequest const& msg)> SendRequestFn;

   ProtobufRouter(SendRequestFn fn);
   void OnPostCommandReply(radiant::tesseract::protocol::PostCommandReply const& reply);

public:     // IRouter
   ReactorDeferredPtr Call(Function const& fn) override;
   ReactorDeferredPtr InstallTrace(Trace const& trace) override;
   ReactorDeferredPtr RemoveTrace(UnTrace const& u);
   void DestroyRemoteTrace(int call_id);

private:
   ReactorDeferredPtr SendNewRequest(radiant::tesseract::protocol::PostCommandRequest& r);

private:
   SendRequestFn           send_request_fn_;
   std::unordered_map<int, ReactorDeferredRef>  pending_calls_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_PROTOBUF_ROUTER_H
