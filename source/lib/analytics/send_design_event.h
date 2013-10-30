#ifndef _RADIANT_LIB_ANALYTICS_SEND_DESIGN_EVENT_H
#define _RADIANT_LIB_ANALYTICS_SEND_DESIGN_EVENT_H

#include "analytics.h"
#include "csg/point.h"
#include "lib/json/node.h"


// usage...
//
//    SendDesignEvent("my:event:name")
//
// or...
//
//    SendDesignEvent("my:event:name").SetValue(5.0f)
//
// or...
//
//    SendDesignEvent("my:event:name")
//       .SetValue(1.0f)
//       .SetArea(csg::Point3f(1, 1, 1))
//

BEGIN_RADIANT_ANALYTICS_NAMESPACE

class SendDesignEvent
{
public:
   SendDesignEvent(std::string const& name);
   ~SendDesignEvent();

   SendDesignEvent& SetValue(float value);
   SendDesignEvent& SetPosition(csg::Point3f const& pt);
   SendDesignEvent& SetArea(std::string const& area);

private:
   json::Node     event_data_node_;
};

END_RADIANT_ANALYTICS_NAMESPACE

#endif // _RADIANT_LIB_ANALYTICS_SEND_DESIGN_EVENT_H
