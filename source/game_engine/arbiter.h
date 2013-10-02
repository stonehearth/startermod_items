#ifndef _RADIANT_GAME_ENGINE_ARBITER_H
#define _RADIANT_GAME_ENGINE_ARBITER_H

#define BOOST_ASIO_PLACEHOLDERS_HPP
#include <boost/asio.hpp>
#include "platform/utils.h"
#include "platform/thread.h"
#include "protocol.h"
#include "client.h"
#include "tesseract.pb.h"
#include "core/singleton.h"
#include <luabind/luabind.hpp>
#include <luabind/class_info.hpp>

using boost::asio::ip::tcp;
namespace boost {
   namespace program_options {
      class options_description;
   }
}

IN_RADIANT_SIMULATION_NAMESPACE(
   class Simulation;
)


namespace radiant {
   namespace game_engine {
      class arbiter : public core::Singleton<arbiter> {
         public:
            arbiter();
            ~arbiter();

            void GetConfigOptions();

            void Run();

         protected:
            void Start();
            void main(); // public for the server.  xxx - there's a better way to factor this between the server and the in-proc listen server
            void mainloop();
            void idle();
            void update_simulation();
            void send_client_updates();
            
            //void OnCellHover(render3d::RendererInterface *renderer, int x, int y, int z);
            //void on_keyboard_pressed(render3d::RendererInterface *renderer, const render3d::keyboard_event &e);

            void process_messages();
            
            void start_accept();
            void handle_accept(std::shared_ptr<tcp::socket> s, const boost::system::error_code& error);
            void SendUpdates(std::shared_ptr<client> c);
            void ProcessSendQueue(std::shared_ptr<client> c);
            void EncodeUpdates(std::shared_ptr<client> c);

         private:
            bool ProcessMessage(std::shared_ptr<client> c, const tesseract::protocol::Request& msg);

         protected:
            boost::asio::io_service             _io_service;
            tcp::socket                         _tcp_listen_socket;
            tcp::acceptor                       _tcp_acceptor;

            std::vector<std::shared_ptr<client>>          _clients;
            int                                 _next_tick;
            int                                 _stepInterval;

            std::unique_ptr<simulation::SimulationInterface>  _simulation;
            platform::timer                     _timer;
            int                                 sequence_number_;
            bool                                paused_;
            struct {
               bool                             noidle;
            }                                   config_;
      };
   };
};

#endif // _RADIANT_GAME_ENGINE_ARBITER_H
