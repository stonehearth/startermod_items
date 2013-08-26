#ifndef _RADIANT_GAME_ENGINE_CLIENT_H
#define _RADIANT_GAME_ENGINE_CLIENT_H

#include <boost/asio.hpp>
#include "simulation/interfaces.h"

using boost::asio::ip::tcp;

namespace radiant {
   namespace game_engine {
      class client {
         public:
            uint32                     sequence_number;
            std::shared_ptr<tcp::socket>    socket;
            simulation::ClientState    state;
            protocol::SendQueuePtr     send_queue;
            protocol::RecvQueuePtr     recv_queue;

         public:
            client();
      };
   };
};

#endif // _RADIANT_GAME_ENGINE_CLIENT_H
