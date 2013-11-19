#ifndef _RADIANT_OM_CLOCK_H
#define _RADIANT_OM_CLOCK_H

#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/set.h"
#include "om/om.h"
#include "om/object_enums.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE


#define CLOCK_TEMPLATE \
   OM_BEGIN_CLASS(Clock, clock, Component) \
      OM_PROPERTY(Clock, time, Time, int) \
   OM_END_CLASS(Clock, clock, Component)

class Clock : public Component
{
public:
   void ConstructObject() override;

   #include "om/reflection/generate_cpp.h"
   CLOCK_TEMPLATE

private:
   #include "om/reflection/generate_record.h"
   CLOCK_TEMPLATE

   #include "om/reflection/generate_record_init.h"
   CLOCK_TEMPLATE
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_CLOCK_H
