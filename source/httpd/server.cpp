#include "radiant.h"
#include "server.h"
#include "client/client.h"
#include "client/rest_api.h"
extern "C" {
#  include "mongoose.h"
}
#include <stdio.h>
#include <stdlib.h>

using namespace ::radiant;
using namespace ::radiant::httpd;

struct mg_callbacks   cb_;

Server::Server() :
   ctx_(NULL)
{
}

Server::~Server()
{
   Stop();
}

// Heap allocate m, condvar, and reply.
// If the current thread times out before the next one comes into cancel
// the request, this may fire and try to use them after they've gone out
// of scope.  ug!
struct ReplyInfo {
   std::mutex m;
   std::condition_variable condvar;
   std::string reply;

   ~ReplyInfo() {
      LOG(WARNING) << "killing reply_info";
   }
};

static int begin_request(struct mg_connection *conn)
{
   void* processed = "yes";
   const struct mg_request_info *request_info = mg_get_request_info(conn);

   std::string uri(request_info->uri);
   std::string query(request_info->query_string ? request_info->query_string : "");
   char postdata[8196];
   int n, len = 0;
   while ((n = mg_read(conn, postdata + len, sizeof(postdata) - len)) > 0) {
      len += n;
   }
   postdata[len] = 0;

   auto reply_info = std::make_shared<ReplyInfo>();
   auto cb = [reply_info](const JSONNode& node) {
      std::lock_guard<std::mutex> guard(reply_info->m);
      reply_info->reply = node.write();
      reply_info->condvar.notify_all();
   };

   
   client::RestAPI* api = nullptr;
   do {
      api = &client::Client::GetInstance().GetAPI();
      if (!api) {
         Sleep(1);
      }
   } while (!api);

   if (api->OnNewRequest(uri, query, postdata, cb)) {
      std::unique_lock<std::mutex> lock(reply_info->m);
      auto timeout = std::chrono::milliseconds(30 * 1000); // xxx: add a config var

      LOG(WARNING) << ">>>>>>> Waiting for " << query << " to complete.";
      reply_info->condvar.wait_for(lock, timeout, [reply_info]() { return !reply_info->reply.empty(); });
      LOG(WARNING) << "<<<<<<< Finished    " << query << "";
      if (!reply_info->reply.empty()) {
         mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                           "Content-Length: %d\r\n"
                           "Content-Type: application/json\r\n"
                           "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                           "Cache-Control: post-check=0, pre-check=0\r\n"
                           "Pragma: no-cache\r\n"
                           "\r\n"
                           "%s\n", reply_info->reply.size(), reply_info->reply.c_str());
         return 1;
      }
   }
   return 0;
}

void Server::Start(const char *docroot, const char* port)
{
   const char *options[] = {
      "num_threads",     "8",
      "document_root",   docroot,
      "listening_ports", port,
      NULL
   };
   memset(&cb_, 0, sizeof cb_);
   //cb_.websocket_ready = websocket_ready_handler;
   //cb_.websocket_data = websocket_data_handler;
   cb_.begin_request = begin_request;
   ctx_ = mg_start(&cb_, NULL, options);
}

void Server::Stop()
{
   if (ctx_) {
      mg_stop(ctx_);
      ctx_ = NULL;
   }
}
