#include "radiant.h"
#include "send_design_event.h"
#include "analytics_logger.h"
#include "lib/json/node.h"


using namespace ::radiant;
using namespace ::radiant::analytics;

const std::string CATEGORY = "design";

//All events are required to have a name by default
SendDesignEvent::SendDesignEvent(std::string const& name)
{
   event_data_node_.set("event_id", name);
}

//Usage is SendDesignEvent(event_name)
//            .SetValue(1.3)
//            .SetPosition(myPosition)
//So send the event in the destructor
SendDesignEvent::~SendDesignEvent()
{
   analytics::AnalyticsLogger &logger = analytics::AnalyticsLogger::GetInstance();
   logger.SubmitLogEvent(event_data_node_, CATEGORY);
}

SendDesignEvent& SendDesignEvent::SetValue(float value)
{
   event_data_node_.set("value", value);
   return *this;
}

SendDesignEvent& SendDesignEvent::SetPosition(csg::Point3f const& pt)
{
   event_data_node_.set("x", pt.x);      
   event_data_node_.set("y", pt.y);      
   event_data_node_.set("z", pt.z);      
   return *this;
}

SendDesignEvent& SendDesignEvent::SetArea(std::string const& area)
{
   event_data_node_.set("area", area);
   return *this;
}