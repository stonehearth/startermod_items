#ifndef _RADIANT_CLIENT_CLOCK_H
#define _RADIANT_CLIENT_CLOCK_H

#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Clock {
public:
   Clock();

   void Update(int game_time, int game_time_interval, int estimated_next_delivery);
   void EstimateCurrentGameTime(int &game_time, float& alpha) const;

private:
   int realtime_start_;
   int game_time_start_;
   int game_time_interval_;
   int game_time_next_delivery_;
   mutable int last_reported_time_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_PLATFORM_UTILS_H
