#include "radiant.h"
#include "radiant_logger.h"
#include "build_number.h"
#include "lib/json/node.h"
#include "analytics_logger.h"
#include "libjson.h"
#include <boost/algorithm/string.hpp>
#include "core/config.h"
#include "post_data.h"
#include "event_data.h"

// Crytop stuff (xxx - change the include path so these generic headers aren't in it)
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "sha.h"
#include "hex.h"
#include "files.h"
#include "md5.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;

using namespace ::radiant;
using namespace ::radiant::analytics;

//Logger begins here
DEFINE_SINGLETON(AnalyticsLogger);

AnalyticsLogger::AnalyticsLogger() :
   stopping_thread_(false),

   // IMPORTANT:
   // Breakpad (the crash reporter) doesn't work with std::thread or poco::thread (poco::thread with dynamic linking might work).
   // They do not have the proper top level exception handling behavior which is configured
   // using SetUnhandledExceptionFilter on Windows.
   // Windows CreateThread and boost::thread appear to work
   post_sender_thread_(std::bind(AnalyticsThreadMain, this))
{   
}

//TODO: test this when there is a way to gracefully exit the game. 
AnalyticsLogger::~AnalyticsLogger()
{
   stopping_thread_ = true;
   cv_.notify_one();
   post_sender_thread_.join();
   // do any other cleanup after thread terminates (none currently)
}

//Called by StartAnalytics session, so we should never get a call to submit log event
//with this set already.
void AnalyticsLogger::SetBasicValues(std::string userid, std::string sessionid, std::string build_version)
{
   userid_ = userid;
   sessionid_ = sessionid;
   build_version_ = build_version;
}

//Construct the full event data and send it to the analytics server
void AnalyticsLogger::SubmitLogEvent(json::Node event_node, std::string event_category)
{
   //If the user has asked us not to collect data, don't. 
   if (!core::Config::GetInstance().GetCollectionStatus()) {
      return;
   }

   ASSERT(!userid_.empty());
   ASSERT(!sessionid_.empty());
   ASSERT(!build_version_.empty());

   //Append common data to the node
   event_node.set("user_id", userid_);
   event_node.set("session_id", sessionid_);
   event_node.set("build", build_version_);

   {
      //Grab the lock
      std::lock_guard<std::mutex> lock(m_);

      //put the node and the category into the queue
      waiting_posts_.push(EventData(event_node, event_category));

      //The lock goes away when lock goes out of scope
      //Note: yes, it is possible for multiple events to be queued up if
      //they fire while the post is stuck
   }

   //notify the conditional 
   cv_.notify_one();
}

//Construct the full event data and send it to the analytics server
void AnalyticsLogger::SubmitPost(json::Node event_node, std::string domain, std::string path, std::string authorization_string)
{
   //If the user has asked us not to collect data, don't. 
   if (!core::Config::GetInstance().GetCollectionStatus()) {
      return;
   }

   ASSERT(!userid_.empty());
   ASSERT(!sessionid_.empty());
   ASSERT(!build_version_.empty());

   //Append common data to the node
   event_node.set("user_id", userid_);
   event_node.set("session_id", sessionid_);
   event_node.set("build", build_version_);

   {
      //Grab the lock
      std::lock_guard<std::mutex> lock(m_);

      //put the node and the category into the queue
      waiting_posts_.push(PostData(event_node, domain, path, authorization_string));

      //The lock goes away when lock goes out of scope
      //Note: yes, it is possible for multiple events to be queued up if
      //they fire while the post is stuck
   }

   //notify the conditional 
   cv_.notify_one();
}

void AnalyticsLogger::SendPostsToServer()
{
   //Local, to hold the events popped off the structure shared b/w threads
   std::queue<PostData> send_posts;

   while(true) {
      //Grab the lock. If there is nothing in the queue and we're not supposed to
      //stop the thread yet, wait. If there is
      //stuff in the queue, or if we're supposed to stop the thread, then continue.
      //Wait to be informed by the main thread that we should be awake
      {
         std::unique_lock<std::mutex> lock(m_);

         if (waiting_posts_.empty() && !stopping_thread_) {
            cv_.wait(lock);
         } 

         ASSERT(send_posts.empty());
         send_posts = std::move(waiting_posts_);
         ASSERT(waiting_posts_.empty());
      }

      //Take all the events in send_posts and post them. 
      //Possible perf. optimization: create all posts and then do curl_multi_perform once at the end
      while (!send_posts.empty()) {
         PostData target_post = send_posts.front();
         send_posts.pop();
         PostJson(target_post);
      }

      //If we're supposed to stop, take this opportunity to do so. 
      if (stopping_thread_) {
         break;
      }
   }
}

void AnalyticsLogger::PostJson(PostData post_data)
{
   json::Node node = post_data.GetJsonNode();
   std::string domain = post_data.GetDomain();
   std::string path = post_data.GetPath();
   std::string authorization_string = post_data.GetAuthorizationString();

   std::string node_string = node.write();

   HTTPClientSession session(domain);
   HTTPRequest request(HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1);
   request.setContentType("text/plain");
   request.add("Authorization", authorization_string);
   request.setContentLength(node_string.length());

   // Send request, returns open stream
   std::ostream& request_stream = session.sendRequest(request);
   request_stream << node_string;

   // Get response
   HTTPResponse response;
   std::istream& response_stream = session.receiveResponse(response);

   // Check result
   int status = response.getStatus();
   if (status != HTTPResponse::HTTP_OK)	{
      // unexpected result code
      LOG(WARNING) << "AnalyticsLogger.PostEvent: Unexpected result code from HTTP POST: " << status;
      LOG(WARNING) << "HTTP POST response was: " << response_stream;
	}
}

// Don't go through Singleton because we would have a race condition with the constructor calling this function
void AnalyticsLogger::AnalyticsThreadMain(AnalyticsLogger* logger)
{
   logger->SendPostsToServer();
}
