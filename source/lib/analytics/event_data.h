#ifndef _RADIANT_ANALYTICS_EVENT_DATA_H
#define _RADIANT_ANALYTICS_EVENT_DATA_H

#include "analytics.h"
#include "lib/json/node.h"

BEGIN_RADIANT_ANALYTICS_NAMESPACE

class EventData 
{
public:
   EventData(json::Node event_node, std::string event_category);
   ~EventData();

   json::Node  GetJsonNode();
   std::string GetCategory();
private:
   json::Node  event_node_;
   std::string event_category_;
};

END_RADIANT_ANALYTICS_NAMESPACE


#endif // _RADIANT_ANALYTICS_EVENT_DATA_H
