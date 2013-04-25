#ifndef _RADIANT_CLIENT_API_H
#define _RADIANT_CLIENT_API_H

#include "namespace.h"
#include "libjson.h"
#include "dm/guard_set.h"
#include "om/om.h"
#include "commands/response.h"
#include "commands/pending_command.h"

#include <mutex>

BEGIN_RADIANT_CLIENT_NAMESPACE

class RestAPI
{
public:
   RestAPI();
   ~RestAPI();

   typedef int SessionId;

   bool OnNewRequest(std::string uri, std::string query, std::string postdata, ResponseFn fn);
   void QueueEvent(std::string evt, JSONNode data);
   void QueueEventFor(SessionId session, std::string evt, JSONNode data);
   void FlushEvents();

   std::vector<PendingCommandPtr> GetPendingCommands();

private:
   void GetEvents(JSONNode args, ResponseFn fn);
   void ExecuteCommand(JSONNode args, ResponseFn fn);
   JSONNode SuccessResponse(JSONNode obj);
   JSONNode ErrorResponse(std::string reason);
   JSONNode PendingReponse();
   void OnSelectionChanged(om::EntityPtr selection);
   int FlushEvents(ResponsePtr response, int when);
   JSONNode ParseQuery(std::string query) const;
   JSONNode FormatEvent(std::string name, JSONNode& data);

private:

   class Session {
   public:
      Session(SessionId id) : id_(id) { }
      void QueueEvent(const JSONNode& e) { events_.push_back(e); }
      void SetResponse(ResponsePtr r);
      void FlushEvents();

   private:
      SessionId                        id_;
      ResponsePtr                      response_;
      std::vector<JSONNode>            events_;
   };
   typedef std::shared_ptr<Session> SessionPtr;

private:
   std::mutex                       lock_;
   int                              nextSessionId_;
   int                              nextTraceId_;
   dm::GuardSet                     traces_;
   std::map<SessionId, SessionPtr>  sessions_;
   std::vector<PendingCommandPtr>   pendingCommands_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_API_H
