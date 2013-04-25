#include "pch.h"
#include "pending_command.h"

using namespace radiant;
using namespace radiant::client;

PendingCommand::PendingCommand(const JSONNode& cmd, ResponsePtr response) :
   cmd_(cmd),
   response_(response)
{
}

std::string PendingCommand::GetCommand() const
{
   return cmd_["command"].as_string();
}

int PendingCommand::GetSession() const
{
   return response_->GetSession();
}

void PendingCommand::SetSession(int session)
{
   response_->SetSession(session);
}

const JSONNode& PendingCommand::GetArgs() const
{
   return cmd_;
}

void PendingCommand::Complete(JSONNode result)
{ 
   response_->Complete(result); 
}

void PendingCommand::Defer(int id) 
{
   response_->Defer(id);
}

void PendingCommand::Error(std::string result)
{
   response_->Error(result); 
}
