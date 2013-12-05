#include "pch.h"
#include "region.h"

using namespace ::radiant;
using namespace ::radiant::om;

// xxx: why isn't all this bundled up in a "trace recursive" package?  Sounds like a good idea!!
dm::GenerationId om::DeepObj_GetLastModified(Region3BoxedPtrBoxed const& boxedRegionPtrField)
{
   dm::GenerationId modified = boxedRegionPtrField.GetLastModified();
   Region3BoxedPtr value = *boxedRegionPtrField;
   if (value) {
      modified = std::max<dm::GenerationId>(modified, value->GetLastModified());
   }
   return modified;
}


// xxx: hmm.  guards are looking more like promises!!! (shared_ptr<Promise> in fact).
DeepRegionGuardPtr om::DeepTraceRegion(Region3BoxedPtrBoxed const& boxedRegionPtrField,
                                       const char* reason,
                                       int category)
{   
   DeepRegionGuardPtr result = std::make_shared<DeepRegionGuard>(boxedRegionPtrField.GetStoreId(),
                                                                 boxedRegionPtrField.GetObjectId());
   DeepRegionGuardRef r = result;

   auto boxed_trace = boxedRegionPtrField.TraceChanges(reason, category);
   result->boxed_trace = boxed_trace;

   boxed_trace->OnChanged([r, reason, category](Region3BoxedPtr value) {
         auto guard = r.lock();
         if (guard) {
            if (value) {
               auto region_trace = value->TraceChanges(reason, category);
               guard->region_trace = region_trace;

               region_trace
                  ->OnChanged([r](csg::Region3 const& region) {
                     auto guard = r.lock();
                     if (guard) {
                        guard->SignalChanged(region);
                     }
                  })
                  ->PushObjectState();
            } else {
               guard->SignalChanged(csg::Region3());
               guard->region_trace = nullptr;
            }
         }
      })
      ->PushObjectState();

   return result;
}

#if 0
Region3BoxedPromise::Region3BoxedPromise(Region3BoxedPtrBoxed const& boxedRegionPtrField, const char* reason)
{
   region_guard_ = DeepTraceRegion(boxedRegionPtrField, reason, [=](csg::Region3 const& r) {
      for (auto& cb : changedCbs_) {
         luabind::call_function<void>(cb, r);
      }
   });
}

Region3BoxedPromise* Region3BoxedPromise::PushChangedCb(luabind::object cb) {
   changedCbs_.push_back(cb);
   return this;
}
#endif
