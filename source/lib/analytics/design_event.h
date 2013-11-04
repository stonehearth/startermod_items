#ifndef _RADIANT_LIB_ANALYTICS_DESIGN_EVENT_H
#define _RADIANT_LIB_ANALYTICS_DESIGN_EVENT_H

#include "analytics.h"
#include "csg/point.h"
#include "lib/json/node.h"


// usage...
//
//    DesignEvent("my:event:name").SendEvent();
//
// or...
//
//    DesignEvent("my:event:name").SetValue(5.0f).SendEvent();
//
// or...
//
//    DesignEvent("my:event:name")
//       .SetValue(1.0f)
//       .SetArea(csg::Point3f(1, 1, 1))
//       .SendEvent();
//
// or...
//
// DesignEvent de = DesignEvent("my:event"name");
// if (foo != "") {
//    de.SetArea(foo);
// }
// de.SendEvent();

BEGIN_RADIANT_ANALYTICS_NAMESPACE

class DesignEvent
{
public:
   DesignEvent(std::string const& name);
   ~DesignEvent();

   DesignEvent& SetValue(float value);
   DesignEvent& SetPosition(csg::Point3f const& pt);
   DesignEvent& SetArea(std::string const& area);

   void DesignEvent::SendEvent();

private:
   json::Node     event_data_node_;
};

END_RADIANT_ANALYTICS_NAMESPACE

#endif // _RADIANT_LIB_ANALYTICS_SEND_DESIGN_EVENT_H
