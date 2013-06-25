#include "pch.h"
#include "response.h"

using namespace radiant;
using namespace radiant::client;

std::atomic_int Response::nextDeferId_;

Response::Response(ResponseFn fn) :
   session_(0),
   responseFn_(fn)
{
   ASSERT(responseFn_);
}

Response::~Response()
{
   if (responseFn_) {
      LOG(WARNING) << "abandoning existing response for session " << session_ << " (not necessarily an error if a timeout occurred in the client).";
   }
}

int Response::GetSession() const
{
   return session_;
}

void Response::SetSession(int session)
{
   session_ = session;
}

void Response::Complete(std::string const& result)
{
   LOG(WARNING) << "Sending: " << result;

   ASSERT(responseFn_);
   responseFn_(result);
   responseFn_ = nullptr;
}

void Response::Defer(int id)
{
   JSONNode response;
   response.push_back(JSONNode("result", "pending"));
   response.push_back(JSONNode("pending_command_id", id));
   Complete(response.write());
}

void Response::Error(std::string const& reason)
{
   JSONNode response;
   response.push_back(JSONNode("result", "error"));
   response.push_back(JSONNode("reason", reason));
   Complete(response.write());
}
