#ifndef _RADIANT_OM_CLOCK_H
#define _RADIANT_OM_CLOCK_H

#include "math3d.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "dm/set.h"
#include "om/om.h"
#include "om/all_object_types.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class Clock : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(Clock, clock);

   void SetTime(int now) { now_ = now; }
   int GetTime() const { return *now_; }

private:
   void InitializeRecordFields() override;

public:
   dm::Boxed<int>             now_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_CLOCK_H
