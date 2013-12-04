#ifndef _RADIANT_PERFMON_TIMELINE_H
#define _RADIANT_PERFMON_TIMELINE_H

#include <unordered_map>
#include "namespace.h"
#include "timer.h"
#include "core/slot.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

class Timeline {
   public:
      Timeline();

      void BeginFrame();

      core::Guard OnFrameEnd(std::function<void(Frame*)> const& fn);
      Counter* GetCurrentCounter();
      Counter* GetCounter(const char* name);
      Counter* SetCounter(Counter* counter);

   private:
      void EndFrame();

   private:
      Frame*               current_frame_;
      Counter*             current_counter_;
      Timer                timer_;
      core::Slot<Frame*>   on_frame_end_slot_;
};

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_TIMELINE_H
