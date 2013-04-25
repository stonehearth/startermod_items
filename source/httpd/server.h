#ifndef _RADIANT_HTTPD_SERVER_H
#define _RADIANT_HTTPD_SERVER_H

#include <condition_variable>
#include "namespace.h"
#include "om/om.h"
#include <vector>

struct mg_context;

BEGIN_RADIANT_HTTPD_NAMESPACE

class Server
{
public:
   Server();
   ~Server();

   void Start(const char *docroot, const char* port);
   void Stop();

private:
   struct mg_context*    ctx_;
};

END_RADIANT_HTTPD_NAMESPACE

#endif //  _RADIANT_HTTPD_SERVER_H
