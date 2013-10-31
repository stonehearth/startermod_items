#include "radiant.h"
#include "send_design_event.h"
#include "analytics_logger.h"
#include "lib/json/node.h"

using namespace ::radiant;
using namespace ::radiant::analytics;

const std::string CATEGORY = "design";

//All events are required to have a name by default
DesignEvent::DesignEvent(std::string const& name)
{
   event_data_node_.set("event_id", name);
}

DesignEvent::~DesignEvent()
{
}

DesignEvent& DesignEvent::SetValue(float value)
{
   event_data_node_.set("value", value);
   return *this;
}

DesignEvent& DesignEvent::SetPosition(csg::Point3f const& pt)
{
   event_data_node_.set("x", pt.x);      
   event_data_node_.set("y", pt.y);      
   event_data_node_.set("z", pt.z);      
   return *this;
}

DesignEvent& DesignEvent::SetArea(std::string const& area)
{
   event_data_node_.set("area", area);
   return *this;
}

void DesignEvent::SendEvent() 
{
   analytics::AnalyticsLogger &logger = analytics::AnalyticsLogger::GetInstance();
   logger.SubmitLogEvent(event_data_node_, CATEGORY);
}