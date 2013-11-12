#ifndef _RADIANT_POST_EVENT_DATA_H
#define _RADIANT_POST_EVENT_DATA_H

#include "analytics.h"
#include "lib/json/node.h"

BEGIN_RADIANT_ANALYTICS_NAMESPACE

class PostData 
{
public:
   PostData(json::Node event_node, std::string uri, std::string authorization_string);
   ~PostData();

   json::Node   GetJsonNode();
   std::string  GetUri();
   std::string  GetAuthorizationString();
   void         Send();
protected:
   json::Node   node_;
   std::string  uri_;
   std::string  authorization_string_;
};

END_RADIANT_ANALYTICS_NAMESPACE


#endif // _RADIANT_ANALYTICS_POST_DATA_H
