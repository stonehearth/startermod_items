#include "radiant.h"
#include "tracer_buffered.h"

using namespace ::radiant;
using namespace ::radiant::dm;

TracerBuffered::~TracerBuffered()
{
}

void TracerBuffered::OnObjectChanged(ObjectId id)
{
   stdutil::UniqueInsert(modified_objects_, id);
}

void TracerBuffered::Flush()
{
   for (ObjectId id : modified_objects_) {
      auto i = traces_.find(id);
      if (i != traces_.end()) {
         stdutil::ForEachPrune<TraceBuffered>(i->second, [](TraceBufferedPtr t) {
            t->Flush();
         });
         if (i->second.empty()) {
            traces_.erase(i);
         }
      }
   }
}

