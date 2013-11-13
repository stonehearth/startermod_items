#ifndef _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H
#define _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H

#include "analytics.h"
#include "event_data.h"
#include "core/singleton.h"
#include "lib/json/node.h"
#include "boost/thread.hpp"

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

   void SetBasicValues(std::string userid, std::string sessionid, std::string build_version);
   void SubmitLogEvent(json::Node event_node, std::string event_category);
   void SubmitPost(json::Node event_node, std::string uri, std::string authorization_string);

private:
   void SendPostsToServer();
   void PostJson(PostData post_data);

   static void AnalyticsThreadMain(AnalyticsLogger* logger);

   std::string userid_;
   std::string sessionid_;
   std::string build_version_;

   std::queue<PostData> waiting_posts_;

   std::mutex m_;
   std::condition_variable cv_;
   boost::thread post_sender_thread_;

   bool stopping_thread_;
};

END_RADIANT_ANALYTICS_NAMESPACE

#endif // _RADIANT_ANALYTICS_ANALYTICS_LOGGER_H
