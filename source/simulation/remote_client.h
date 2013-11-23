#ifndef _RADIANT_GAME_ENGINE_CLIENT_H
#define _RADIANT_GAME_ENGINE_CLIENT_H

#include <boost/asio.hpp>
#include "namespace.h"
#include "protocol.h"
#include "dm/dm.h"

using boost::asio::ip::tcp;

BEGIN_RADIANT_SIMULATION_NAMESPACE

class RemoteClient {
public:
   std::shared_ptr<tcp::socket>  socket;
   protocol::SendQueuePtr        send_queue;
   protocol::RecvQueuePtr        recv_queue;
   dm::StreamerPtr               streamer;

public:
   RemoteClient();
};

END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_GAME_ENGINE_CLIENT_H
