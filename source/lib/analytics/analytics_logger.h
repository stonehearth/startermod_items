#ifndef _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H
#define _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H

#include "analytics.h"
#include "event_data.h"
#include "core/singleton.h"
#include "core/thread.h"
#include "lib/json/node.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>


BEGIN_RADIANT_ANALYTICS_NAMESPACE

class AnalyticsLogger : public core::Singleton<AnalyticsLogger>
{
public:
   AnalyticsLogger();
   ~AnalyticsLogger();

   void SetBasicValues(std::string userid, std::string sessionid, std::string build_version, bool collect_analytics);
   void SubmitLogEvent(json::Node event_node, std::string event_category);

private:
   void SendEventsToServer();
   void PostEvent(json::Node event_node, std::string event_category);

   static void AnalyticsThreadMain();

   std::string userid_;
   std::string sessionid_;
   std::string build_version_;
   bool collect_analytics_;

   std::queue<EventData> waiting_events_;

   std::mutex m_;
   std::condition_variable cv_;
   radiant::core::Thread event_sender_thread_;

   bool stopping_thread_;

};

END_RADIANT_ANALYTICS_NAMESPACE

#endif // _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H
