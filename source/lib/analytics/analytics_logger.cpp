#include "radiant.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "analytics_logger.h"
#include "libjson.h"
#include <boost/algorithm/string.hpp>  
#include "core/config.h"

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

const std::string ANALYTICS_DOMAIN = "api.gameanalytics.com";

//Keys for the stonehearth-dev game project. Leave in while debugging
const std::string GAME_KEY = "2b6cc12b9457de0ae969e0d9f8b04291";
const std::string SECRET_KEY = "70904f041d9e579c3d34f40cdb5bc0c16ad0c09a";

//These are the actual keys. Comment in for release (or with a compiler option)
//const std::string GAME_KEY = "5777a99f437a7ea5ade1548b7b3d8cde";
//const std::string SECRET_KEY = "bf16a98c575fb0b166c84ae6924c4b15d01b3901";

const std::string API_VERSION = "1";

//Logger begins here
DEFINE_SINGLETON(AnalyticsLogger);

AnalyticsLogger::AnalyticsLogger() :
   stopping_thread_(false),
   event_sender_thread_(AnalyticsThreadMain)
{   
}

//TODO: test this when there is a way to gracefully exit the game. 
AnalyticsLogger::~AnalyticsLogger()
{
   stopping_thread_ = true;
   cv_.notify_one();
   event_sender_thread_.Join();
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
      waiting_events_.push(EventData(event_node, event_category));

      //The lock goes away when lock goes out of scope
      //Note: yes, it is possible for multiple events to be queued up if
      //they fire while the post is stuck
   }

   //notify the conditional 
   cv_.notify_one();
}

void AnalyticsLogger::SendEventsToServer()
{
   //Local, to hold the events popped off the structure shared b/w threads
   std::queue<EventData> send_events;

   while(true) {
      //Grab the lock. If there is nothing in the queue and we're not supposed to
      //stop the thread yet, wait. If there is
      //stuff in the queue, or if we're supposed to stop the thread, then continue.
      //Wait to be informed by the main thread that we should be awake
      {
         std::unique_lock<std::mutex> lock(m_);

         if (waiting_events_.empty() && !stopping_thread_) {
            cv_.wait(lock);
         } 

         ASSERT(send_events.empty());
         send_events = std::move(waiting_events_);
         ASSERT(waiting_events_.empty());
      }

      //Take all the events in send_events and post them. 
      //Possible perf. optimization: create all posts and then do curl_multi_perform once at the end
      while (!send_events.empty()) {
         EventData target_event = send_events.front();
         send_events.pop();
         PostEvent(target_event.GetJsonNode(), target_event.GetCategory());
      }

      //If we're supposed to stop, take this opportunity to do so. 
      if (stopping_thread_) {
         break;
      }
   }
}

void AnalyticsLogger::PostEvent(json::Node event_node, std::string event_category)
{
   std::string event_string = "{\"event_id\": \"game:is_running\", \"user_id\": \"a9c82cd2-4e98-42a0-b022-6051f7cdb55f\", \"build\": \"preview_0.314a\", \"session_id\": \"399db052-6850-4d0c-ba38-d4d442efc473\"}";
   //std::string event_string = event_node.write(); // CHECKCHECK

   // the header is the data + secret key
   std::string header = event_string + SECRET_KEY;

   // Hash the header with md5
   // Borrowing sample code from http://www.cryptopp.com/wiki/Hash_Functions
   CryptoPP::Weak::MD5 hash;
   byte digest[CryptoPP::Weak::MD5::DIGESTSIZE];
   hash.CalculateDigest(digest, (byte*)header.c_str(), header.length());

   CryptoPP::HexEncoder encoder;
   std::string digest_string;
   encoder.Attach(new CryptoPP::StringSink(digest_string));
   encoder.Put(digest, sizeof(digest));
   encoder.MessageEnd();

   // Super important to make sure the final hex is all lower case
   boost::algorithm::to_lower(digest_string);

   std::string path = "/" + API_VERSION + "/" + GAME_KEY + "/" + event_category;

   HTTPClientSession session(ANALYTICS_DOMAIN);

   HTTPRequest request(HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1);
   request.setContentType("text/plain");
   request.add("Authorization", digest_string);
   request.setContentLength(event_string.length());

   // Send request, returns open stream
   std::ostream& request_stream = session.sendRequest(request);
   request_stream << event_string;

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

void AnalyticsLogger::AnalyticsThreadMain()
{
   AnalyticsLogger::GetInstance().SendEventsToServer();
}
