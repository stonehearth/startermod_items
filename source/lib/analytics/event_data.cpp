#include "radiant.h"
#include "radiant_logger.h"

#include "event_data.h"
#include "lib/json/node.h"

using namespace ::radiant;
using namespace ::radiant::analytics;

//Helper data class, Event Data
EventData::EventData(json::Node event_node, std::string event_category)
{
   event_node_ = event_node;
   event_category_ = event_category;
}

EventData::~EventData()
{
}

json::Node EventData::GetJsonNode()
{
   return event_node_;
}

std::string EventData::GetCategory()
{
   return event_category_;
}