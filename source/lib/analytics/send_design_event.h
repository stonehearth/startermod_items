#ifndef _RADIANT_LIB_ANALYTICS_SEND_DESIGN_EVENT_H
#define _RADIANT_LIB_ANALYTICS_SEND_DESIGN_EVENT_H

#include "analytics.h"
#include "csg/point.h"

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
   std::string    category_;
   std::string    event_id_;
   std::string    area_;

   float          value_num_;
   csg::Point3f   position_;

   //We assume that "" means nothing has been set for strings
   //So we want bools to determine if the others have been set by the user
   //Review Q: If this doesn't work stylistically we could make position a pointer instead
   bool           set_value_;
   bool           set_position_;
};

END_RADIANT_ANALYTICS_NAMESPACE

#endif // _RADIANT_LIB_ANALYTICS_SEND_DESIGN_EVENT_H
