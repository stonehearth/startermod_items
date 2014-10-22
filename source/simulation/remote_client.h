#ifndef _RADIANT_GAME_ENGINE_CLIENT_H
#define _RADIANT_GAME_ENGINE_CLIENT_H

#include <boost/asio.hpp>
#include "namespace.h"
#include "protocol.h"
#include "dm/dm.h"

using boost::asio::ip::tcp;

BEGIN_RADIANT_SIMULATION_NAMESPACE

class RemoteClient
{
public:
   RemoteClient();

   bool IsClientReadyForUpdates() const;
   void SetSequenceNumber(int sequenceNumber);
   void AckSequenceNumber(int sequenceNumber);
   void FlushSendQueue();

public:  // Someone should get shot for making these public (probably the person typing this comment right now...)
   std::shared_ptr<tcp::socket>  socket;
   protocol::SendQueuePtr        send_queue;
   protocol::RecvQueuePtr        recv_queue;
   dm::StreamerPtr               streamer;

private:
   mutable bool   _lastReadyToRun;
   int            _lastSequenceNumberAckd;
   int            _lastSequenceNumberSent;
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_GAME_ENGINE_CLIENT_H
