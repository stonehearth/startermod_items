#include "radiant.h"
#include "store.h"
#include "timeline.h"

using namespace radiant;
using namespace radiant::perfmon;

Store::Store() :
   timeline_(new Timeline)
{
}

Timeline* Store::GetTimeline()
{
   return timeline_.get();
}
