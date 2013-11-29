#include "radiant.h"
#include "core/config.h"
#include "arbiter.h"
#include "metrics.h"
#include "simulation.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
extern "C" {
    #include "lauxlib.h"
    #include "lualib.h"
}

using ::radiant::platform::Thread;
using ::radiant::simulation::Simulation;
using namespace ::radiant;
using namespace ::radiant::game_engine;

namespace po = boost::program_options;
namespace proto = ::radiant::tesseract::protocol;

DEFINE_SINGLETON(arbiter);

static void* LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize)
{
   if (nsize == 0) {
      free(ptr);
      return NULL;
   }
   return realloc(ptr, nsize);
}

arbiter::arbiter() :
   _next_tick(0),
   sequence_number_(1),
   paused_(false),
   _tcp_acceptor(nullptr)
{
}

void arbiter::GetConfigOptions()
{
   // "suspend the idle loop, running the game as fast as possible."
   config_.noidle = core::Config::GetInstance().Get("game.noidle", false);
}

arbiter::~arbiter()
{
}

void arbiter::Run(tcp::acceptor* acceptor, boost::asio::io_service* io_service)
{
   _tcp_acceptor = acceptor;
   _io_service = io_service;
   GetConfigOptions();
   Start();
   main();
}

void arbiter::Start()
{
   _simulation.reset(::radiant::simulation::CreateSimulation());
}

bool arbiter::ProcessMessage(std::shared_ptr<client> c, const proto::Request& msg)
{
   return _simulation->ProcessMessage(msg, c->send_queue);
}

void arbiter::handle_accept(std::shared_ptr<tcp::socket> socket, const boost::system::error_code& error)
{
   if (!error) {
      std::shared_ptr<client> c(new client());

      c->socket = socket;
      c->send_queue = protocol::SendQueue::Create(*socket);
      c->recv_queue = std::make_shared<protocol::RecvQueue>(*socket);
      c->recv_queue->Read();

      _clients.push_back(c);
      //queue_network_read(c, protocol::alloc_buffer(1024 * 1024));
   }
   start_accept();
}

void arbiter::start_accept()
{
   std::shared_ptr<tcp::socket> socket(new tcp::socket(*_io_service));
   auto cb = bind(&arbiter::handle_accept, this, socket, std::placeholders::_1);
   _tcp_acceptor->async_accept(*socket, cb);
}

void arbiter::main()
{
   _tcp_acceptor->listen();
   start_accept();

   _simulation->CreateNew();

   unsigned int last_stat_dump = 0;

   _stepInterval = 1000 / 20;
   _timer.set(_stepInterval);

   while (1) {
      mainloop();

      unsigned int now = platform::get_current_time_in_ms();
      if (now - last_stat_dump > 3000) {
         //PROFILE_DUMP_STATS();
         PROFILE_RESET();
         last_stat_dump = now;
      }
   }
}

void arbiter::mainloop()
{
   PROFILE_BLOCK();

   process_messages();

   if (!paused_) {
      update_simulation();
   }
   send_client_updates();
   idle();
}

volatile int idlecount = 1;

void arbiter::idle()
{
   PROFILE_BLOCK();

   // LOG(WARNING) << _timer.remaining() << " time left in idle";

   // xxx: we should probably read and parse network events even while idle.

   _simulation->Idle(_timer);

   if (!config_.noidle) {
      while (!_timer.expired()) {
         platform::sleep(1);
      }
   }
   _timer.set(_stepInterval + _timer.remaining());
}

void arbiter::update_simulation()
{
   PROFILE_BLOCK();
   _simulation->Step(_timer, _stepInterval);
}

void arbiter::send_client_updates()
{
   PROFILE_BLOCK();
   for (std::shared_ptr<client> c : _clients) {
      SendUpdates(c);
   };
}

void arbiter::SendUpdates(std::shared_ptr<client> c)
{
   EncodeUpdates(c);
   ProcessSendQueue(c);
}

void arbiter::ProcessSendQueue(std::shared_ptr<client> c) 
{
   protocol::SendQueue::Flush(c->send_queue);
}

void arbiter::EncodeUpdates(std::shared_ptr<client> c)
{
   // Update the sequence number...
   proto::Update update;
   update.set_type(proto::Update::BeginUpdate);
   auto msg = update.MutableExtension(proto::BeginUpdate::extension);
   msg->set_sequence_number(sequence_number_);
   c->send_queue->Push(protocol::Encode(update));
   c->sequence_number = sequence_number_;

   // Get the updates from the simulation
   _simulation->EncodeUpdates(c->send_queue, c->state);

   // End the sequence
   update.Clear();
   update.set_type(proto::Update::EndUpdate);
   c->send_queue->Push(protocol::Encode(update));
}

void arbiter::process_messages()
{
   PROFILE_BLOCK();

   _io_service->poll();
   _io_service->reset();

   for (auto c : _clients) {
      c->recv_queue->Process<proto::Request>([=](proto::Request const& msg) -> bool {
         return ProcessMessage(c, msg);
      });
   }
}
