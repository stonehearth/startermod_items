#ifndef _RADIANT_LIB_RPC_PROTOBUF_REACTOR_H
#define _RADIANT_LIB_RPC_PROTOBUF_REACTOR_H

#include <unordered_map>
#include "namespace.h"
#include "radiant_json.h"
#include "tesseract.pb.h"
#include "lib/rpc/forward_defines.h"

BEGIN_RADIANT_RPC_NAMESPACE

class ProtobufReactor {
public:
   typedef std::function<void(radiant::tesseract::protocol::PostCommandReply const&)> SendReplyFn;

   ProtobufReactor(CoreReactor &core, SendReplyFn send_reply_fn);

   ReactorDeferredPtr Dispatch(SessionPtr session, radiant::tesseract::protocol::PostCommandRequest const& msg);

private:
   CoreReactor &core_;
   SendReplyFn send_reply_fn_;
   std::unordered_map<int, ReactorDeferredPtr>  traces_;
};

END_RADIANT_RPC_NAMESPACE

#endif // _RADIANT_LIB_RPC_REACTOR_H
