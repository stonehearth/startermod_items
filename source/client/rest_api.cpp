#include "pch.h"
#include "rest_api.h"
#include "client.h"
#include "om/entity.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

using namespace radiant;
using namespace radiant::client;

RestAPI::RestAPI() :
   nextSessionId_(1),
   nextTraceId_(1)
{
   traces_ += Client::GetInstance().TraceSelection(std::bind(&RestAPI::OnSelectionChanged, this, std::placeholders::_1));
}

RestAPI::~RestAPI()
{
}

bool RestAPI::OnNewRequest(std::string uri, std::string query, std::string postdata, ResponseFn cb)
{
   std::lock_guard<std::mutex> guard(lock_);

   if (uri == "/api/events") {
      GetEvents(ParseQuery(query), cb);
      return true;
   }
   if (uri == "/api/commands/execute") {
      JSONNode args;
      try {
         args = libjson::parse(postdata);
      } catch (const std::invalid_argument& e) {
         LOG(WARNING) << "invalid postdata in RestAPI::OnNewRequest: " << e.what();
         return false;
      }
      ExecuteCommand(args, cb);
      return true;
   }
   if (boost::starts_with(uri, "/object")) {
      ResponsePtr response = std::make_shared<Response>(cb);
      JSONNode args;
      args.push_back(JSONNode("command", "fetch_object")); // xxx: this is crap.  should be functor or something!  see Client::ExecuteCommands for the super crappy part.
      args.push_back(JSONNode("uri", uri));
      pendingCommands_.push_back(std::make_shared<PendingCommand>(args, response));
      return true;
   }
   return false;
}

// may be called from any thread
void RestAPI::QueueEvent(std::string evt, JSONNode data)
{
   std::lock_guard<std::mutex> guard(lock_);

   JSONNode e = FormatEvent(evt, data);
   for (const auto& session : sessions_) {
      session.second->QueueEvent(e);
   }
}

void RestAPI::QueueEventFor(SessionId id, std::string evt, JSONNode data)
{
   std::lock_guard<std::mutex> guard(lock_);

   auto i = sessions_.find(id);
   if (i != sessions_.end()) {
      i->second->QueueEvent(FormatEvent(evt, data));
   }
}

JSONNode RestAPI::FormatEvent(std::string name, JSONNode& data)
{
   JSONNode e(JSON_NODE);
   e.push_back(JSONNode("type", name));
   e.push_back(JSONNode("when", Client::GetInstance().Now()));

   data.set_name("data");
   e.push_back(data);

   return e;
}

void RestAPI::GetEvents(JSONNode args, ResponseFn completeCb)
{
   ResponsePtr response = std::make_shared<Response>(completeCb);

   CHECK_TYPE(response, args, "endpoint_id", JSON_NUMBER);

   SessionPtr session;
   SessionId sessionId = (SessionId)args["endpoint_id"].as_int();

   if (!sessionId) {
      sessionId = nextSessionId_++;
      sessions_[sessionId] = session = std::make_shared<Session>(sessionId);
      
      JSONNode reply, data;
      reply.push_back(JSONNode("type", "radiant.events.set_endpoint"));
      data.set_name("data");
      data.push_back(JSONNode("endpoint_id", sessionId));
      reply.push_back(data);

      session->QueueEvent(reply);
   } else {
      auto i = sessions_.find(sessionId);
      if (i != sessions_.end()) {
         session = i->second;
      }
   }
   if (!session) {
      response->Error("unknonwn session id " + stdutil::ToString(sessionId));
      return;
   }
   session->SetResponse(response);
}

void RestAPI::ExecuteCommand(JSONNode args, ResponseFn completeCb)
{
   LOG(WARNING) << "Received command:  " << args.write_formatted();

   ResponsePtr response = std::make_shared<Response>(completeCb);
   CHECK_TYPE(response, args, "command", JSON_STRING);
   CHECK_TYPE(response, args, "endpoint_id", JSON_NUMBER);

   SessionId sessionId = args["endpoint_id"].as_int();
   auto i = sessions_.find(sessionId);
   if (i == sessions_.end()) {
      response->Error("Unknown session id " + stdutil::ToString(sessionId));
      return;
   }
   response->SetSession(sessionId);
   pendingCommands_.push_back(std::make_shared<PendingCommand>(args, response));
}


// will be called by the client thread
void RestAPI::OnSelectionChanged(om::EntityPtr selection)
{
   std::string uri;
   if (selection) {
      uri = std::string("/object/") + stdutil::ToString(selection->GetObjectId());
   }
   JSONNode data(JSON_NODE);
   data.push_back(JSONNode("selected_entity", uri));

   QueueEvent("radiant.events.selection_changed", data);
}

// will be called from the client thread
std::vector<PendingCommandPtr> RestAPI::GetPendingCommands()
{
   std::lock_guard<std::mutex> guard(lock_);
   return std::move(pendingCommands_);
}

// will be called from the client thread
void RestAPI::FlushEvents()
{
   std::lock_guard<std::mutex> guard(lock_);

   for (const auto& session : sessions_) {
      session.second->FlushEvents();
   }
}

void RestAPI::Session::SetResponse(ResponsePtr r)
{
   response_ = r;
   response_->SetSession(id_);
   FlushEvents();
}

void RestAPI::Session::FlushEvents()
{
   if (response_ && !events_.empty()) {
      JSONNode result(JSON_ARRAY);
      for (const auto& e : events_) {
         result.push_back(e);
      }
      events_.clear();
      response_->Complete(result.write());
      response_ = nullptr;
   }
}


JSONNode RestAPI::ParseQuery(std::string query) const
{
   JSONNode result;
   size_t start = 0, end;
   do {
      end = query.find('&', start);
      if (end == std::string::npos) {
         end = query.size();
      }
      if (end > start) {
         size_t mid = query.find_first_of('=', start);
         if (mid != std::string::npos) {
            std::string name = query.substr(start, mid - start);
            std::string value = query.substr(mid + 1, end - mid - 1);

            int ivalue = atoi(value.c_str());
            if (ivalue != 0 || value == "0") {
               result.push_back(JSONNode(name, ivalue));
            } else {
               result.push_back(JSONNode(name, value));
            }
         }
         start = end + 1;
      }
   } while (end < query.size());
   return result;
}
