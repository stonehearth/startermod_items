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

void Response::Complete(JSONNode result)
{
   JSONNode response;
   response.push_back(JSONNode("result", "success"));

   result.set_name("response");
   response.push_back(result);
   Deliver(response);
}

void Response::Defer(int id)
{
   JSONNode response;
   response.push_back(JSONNode("result", "pending"));
   response.push_back(JSONNode("pending_command_id", id));
   Deliver(response);
}

void Response::Error(std::string reason)
{
   JSONNode response;
   response.push_back(JSONNode("result", "error"));
   response.push_back(JSONNode("reason", reason));
   Deliver(response);

}

void Response::Deliver(const JSONNode& response)
{
   LOG(WARNING) << "Sending: " << response.write_formatted();

   ASSERT(responseFn_);
   responseFn_(response);
   responseFn_ = nullptr;
}
