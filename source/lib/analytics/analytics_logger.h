#ifndef _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H
#define _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H

#include "analytics.h"
#include "core/singleton.h"
#include "lib/json/node.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>


BEGIN_RADIANT_ANALYTICS_NAMESPACE

class EventData 
{
public:
   EventData(json::Node event_node, std::string event_category);
   ~EventData();

   json::Node  GetEventJson();
   std::string GetEventType();
private:
   json::Node  event_node_;
   std::string event_category_;
};

class AnalyticsLogger : public core::Singleton<AnalyticsLogger>
{
public:
   AnalyticsLogger();
   ~AnalyticsLogger();

   void SetBasicValues(std::string userid, std::string sessionid, std::string build_version);
   void SubmitLogEvent(json::Node event_node, std::string event_category);

private:
   void SendEventsToServer();
   void PostEvent(json::Node event_node, std::string event_category);

   std::string userid_;
   std::string sessionid_;
   std::string build_version_;

   std::queue<EventData*> waiting_events_;

   std::mutex m_;
   std::condition_variable cv_;
   std::thread* event_sender_thread_;

};

END_RADIANT_ANALYTICS_NAMESPACE

#endif // _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H
