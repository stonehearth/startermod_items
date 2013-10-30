#include "radiant.h"
#include "radiant_logger.h"
#include "lib/json/node.h"
#include "analytics_logger.h"
#include "libjson.h"
#include <boost/algorithm/string.hpp>  

#include "curl/curl.h"
#include "curl/easy.h"
#include "curl/curlbuild.h"

// Crytop stuff (xxx - change the include path so these generic headers aren't in it)
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "sha.h"
#include "hex.h"
#include "files.h"
#include "md5.h"

static size_t write_to_string(void *ptr, size_t size, size_t count, void *stream);


using namespace ::radiant;
using namespace ::radiant::analytics;

const std::string WEBSITE_URL = "http://api.gameanalytics.com/";

//Keys for the stonehearth-dev game project. Leave in while debugging
const std::string GAME_KEY = "2b6cc12b9457de0ae969e0d9f8b04291";
const std::string SECRET_KEY = "70904f041d9e579c3d34f40cdb5bc0c16ad0c09a";

//These are the actual keys. Comment in for release (or with a compiler option)
//const std::string GAME_KEY = "5777a99f437a7ea5ade1548b7b3d8cde";
//const std::string SECRET_KEY = "bf16a98c575fb0b166c84ae6924c4b15d01b3901";

const std::string API_VERSION = "1";

//Helper data class, Event Data
EventData::EventData(json::Node event_node, std::string event_category)
{
   event_node_ = event_node;
   event_category_ = event_category;
}

EventData::~EventData()
{
}

json::Node EventData::GetEventJson()
{
   return event_node_;
}

std::string EventData::GetEventType()
{
   return event_category_;
}

//Logger begins here
DEFINE_SINGLETON(AnalyticsLogger);

AnalyticsLogger::AnalyticsLogger() 
{   
   // Initialize libcurl.
   //(note, not threadsafe, so always call in application.cpp.)
   //See: http://curl.haxx.se/libcurl/c/curl_global_init.html
   curl_global_init(CURL_GLOBAL_ALL);

   //Start the sending thread
   //Reference: http://stackoverflow.com/questions/10673585/start-thread-with-member-function
   event_sender_thread_ = new std::thread(&AnalyticsLogger::SendEventsToServer, this);
}

AnalyticsLogger::~AnalyticsLogger()
{
   delete event_sender_thread_;
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
      waiting_events_.push(new EventData(event_node, event_category));

      //The lock goes away when lock goes out of scope
      //TODO: is it even possible for there to be more than one event to be passed this way?
   }

   //notify the conditional 
   cv_.notify_one();
}

void AnalyticsLogger::SendEventsToServer()
{
   //Local, to hold the events popped off the structure shared b/w threads
   std::queue<EventData*> send_events;

   while(true) {
      //Grab the lock. If there is nothing in the queue, wait. If there is
      //stuff in the queue, then continue.
      //Wait to be informed by the main thread that we should be awake
      {
         std::unique_lock<std::mutex> lock(m_);

         if (waiting_events_.empty()) {
            cv_.wait(lock);
         } 
         //There should totally be something in the queue now
         while (!waiting_events_.empty()) {
            EventData* target_event = waiting_events_.front();
            waiting_events_.pop();
            send_events.push(target_event);
         }
      }

      //Take all the events in send_events and post them. 
      while (!send_events.empty()) {
         EventData* target_event = send_events.front();
         send_events.pop();
         PostEvent(target_event->GetEventJson(), target_event->GetEventType());
      }
   }
}

void AnalyticsLogger::PostEvent(json::Node event_node, std::string event_category)
{
   //Do we want to try the multi-handle and send multiple posts? or just stay simple?
   //Create the async-multi-handle
   //CURLM* multi_handle = NULL;

   //Create a new CURL object
   CURL* curl = curl_easy_init();
   if (curl) {
      std::string url = WEBSITE_URL + API_VERSION + "/" + GAME_KEY + "/" + event_category;

      std::string event_string = event_node.write();

      //the header is the data + secret key
      std::string header = event_string + SECRET_KEY;

      //Hash the header with md5
      //Borrowing sample code from http://www.cryptopp.com/wiki/Hash_Functions
      CryptoPP::Weak::MD5 hash;
      byte digest[CryptoPP::Weak::MD5::DIGESTSIZE];
      hash.CalculateDigest(digest, (byte*)header.c_str(), header.length());

      CryptoPP::HexEncoder encoder;
      std::string output;
      encoder.Attach(new CryptoPP::StringSink(output));
      encoder.Put(digest, sizeof(digest));
      encoder.MessageEnd();

      //Super important to make sure the final hex is all lower case
      boost::algorithm::to_lower(output);
      std::string authorization_str = "Authorization: " + output;

      //Create  struct of headers, so we can in the future, attach multiple headers
      struct curl_slist *chunk = NULL;
      chunk = curl_slist_append(chunk, authorization_str.c_str());
      curl_slist_append(chunk, "Content-Type: text/plain");

      //OK, init curl to have all the post data
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, event_string.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      curl_easy_setopt(curl, CURLOPT_POST, 1);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20000);

      //If we get a response from the server, put it in response
      std::string response;
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

      //Send the post request
      CURLcode res = curl_easy_perform(curl);

      //Send the post async
      // multi_handle = curl_multi_init();
      //CURLMcode res = curl_multi_add_handle(multi_handle, curl);

      //Each call to multi_perform updates the # of handles still running, and returns
      //whether we should call the function again. Stop when there are no handles and no
      //more need to call the function
      //TODO: how to yield to other threads?
      //int handles_still_running = 0;
      //do {
      //   while (curl_multi_perform(multi_handle, &handles_still_running) == CURLM_CALL_MULTI_PERFORM);
      //} while (handles_still_running);

      if (res == CURLE_OK) {
         LOG(INFO) << "Post worked! Status is: " << response;
      } else {
         //TODO: log the error
         LOG(WARNING) << "Post didn't work. Status is: " << res;
      }
      
      //Cleanup, need one for every curl init
      curl_easy_cleanup(curl);
      curl_slist_free_all(chunk);
   }
}

//Helper function to write response data
size_t write_to_string(void *ptr, size_t size, size_t count, void *stream)
{
  ((std::string*)stream)->append((char*)ptr, 0, size*count);
  return size*count;
}