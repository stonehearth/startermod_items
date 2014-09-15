#include "radiant.h"
#include "profiler.h"

#if defined(VTUNE_PATH)
#include "ittnotify.h"
#endif

using namespace ::radiant;
using namespace ::radiant::core;

static bool enabled = false;

bool core::IsProfilerAvailable()
{
#if defined(VTUNE_PATH)
   return true;
#else
   return false;
#endif
}

bool core::IsProfilerEnabled()
{
   return enabled;
}

void core::SetProfilerEnabled(bool value)
{
#if defined(VTUNE_PATH)
   if (enabled != value) {
      enabled = value;
      if (enabled) {
         __itt_resume();
      } else {
         __itt_pause();
      }
   }
#endif
}
