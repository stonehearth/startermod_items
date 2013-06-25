#ifndef _RADIANT_SIMULATION_INTERFACES_H
#define _RADIANT_SIMULATION_INTERFACES_H

#include "platform/namespace.h"
#include "tesseract.pb.h"
#include "namespace.h"
#include "protocol.h"
#include <luabind/luabind.hpp>

IN_RADIANT_PLATFORM_NAMESPACE (
   class timer;
)

BEGIN_RADIANT_SIMULATION_NAMESPACE


struct ClientState {
   int last_update;

   ClientState() { Reset(); }
   void Reset() { last_update = 0; }
};

class SimulationInterface {
public:
   virtual ~SimulationInterface();

   virtual void CreateNew() = 0;
   virtual void Save(std::string id) = 0;
   virtual void Load(std::string id) = 0;
   virtual void ProcessCommand(const ::radiant::tesseract::protocol::Cmd &cmd) = 0;
   virtual void ProcessCommand(::google::protobuf::RepeatedPtrField<tesseract::protocol::Reply >* replies, const ::radiant::tesseract::protocol::Command &command) = 0;
   virtual void EncodeUpdates(protocol::SendQueuePtr queue, ClientState& cs) = 0;
   virtual void Step(platform::timer &timer, int interval) = 0;
   virtual void Idle(platform::timer &timer) = 0;
   virtual void DoAction(const tesseract::protocol::DoAction& msg, protocol::SendQueuePtr queue) = 0;
   virtual void FetchObject(tesseract::protocol::FetchObjectRequest const& request, tesseract::protocol::FetchObjectReply* reply) = 0;
};

SimulationInterface *CreateSimulation(lua_State* L);

END_RADIANT_SIMULATION_NAMESPACE

#endif //  _RADIANT_SIMULATION_INTERFACES_H
