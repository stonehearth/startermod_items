#include "radiant.h"
#include "send_design_event.h"
#include "analytics_logger.h"
#include "lib/json/node.h"


using namespace ::radiant;
using namespace ::radiant::analytics;

//All events are required to have a name by default
SendDesignEvent::SendDesignEvent(std::string const& name) :
   set_value_(false),
   set_position_(false),
   area_(""),
   category_("design")
{
   event_id_ = name;
}

//Usage is SendDesignEvent(event_name)
//            .SetValue(1.3)
//            .SetPosition(myPosition)
//So send the event in the destructor
SendDesignEvent::~SendDesignEvent()
{
   //Make a json node out of the stuff
   json::Node node;
   node.set("event_id", event_id_);
   
   if (area_ != "") {
      node.set("area", area_);
   }

   if (set_position_) {
      node.set("x", position_.x);      
      node.set("y", position_.y);      
      node.set("z", position_.z);      
   }

   if (set_value_) {
      node.set("value", value_num_);
   }

   analytics::AnalyticsLogger &logger = analytics::AnalyticsLogger::GetInstance();
   logger.SubmitLogEvent(node, category_);
}

SendDesignEvent& SendDesignEvent::SetValue(float value)
{
   value_num_ = value;
   set_value_ = true;
   return *this;
}

SendDesignEvent& SendDesignEvent::SetPosition(csg::Point3f const& pt)
{
   position_ = pt;
   set_position_ = true;
   return *this;
}

SendDesignEvent& SendDesignEvent::SetArea(std::string const& area)
{
   area_ = area;
   return *this;
}