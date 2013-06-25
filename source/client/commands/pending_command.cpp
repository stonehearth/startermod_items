#include "pch.h"
#include "pending_command.h"

using namespace radiant;
using namespace radiant::client;

PendingCommand::PendingCommand(const JSONNode& cmd, ResponsePtr response) :
   cmd_(cmd),
   response_(response)
{
}

int PendingCommand::GetSession() const
{
   return response_->GetSession();
}

void PendingCommand::SetSession(int session)
{
   response_->SetSession(session);
}

void PendingCommand::CompleteSuccessObj(JSONNode const& obj)
{
   JSONNode success;
   success.push_back(JSONNode("result", "success"));

   JSONNode response = obj;
   response.set_name("response");
   response.push_back(response);

   Complete(success.write());     
}
