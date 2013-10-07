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
                                                  std::function<void(csg::Region3 const& r)> updateCb)
{   
   auto fieldValueChangedCb = [=, &boxedRegionPtrField](Region3BoxedRef v) {
      Region3BoxedPtr value = v.lock();
      if (value) {
         LOG(INFO) << "boxed-boxed-region-ptr's inner box modified (!)";
         LOG(INFO) << dm::DbgInfo::GetInfoString(*value);
         updateCb(value->Get());
      }
   };

   DeepRegionGuardPtr result = std::make_shared<DeepRegionGuard>(boxedRegionPtrField.GetStoreId(), boxedRegionPtrField.GetObjectId());
   DeepRegionGuardRef r = result;
   auto fieldChangedCb = [=, &boxedRegionPtrField]() {
      DeepRegionGuardPtr g = r.lock();
      if (g) {
         LOG(INFO) << "boxed-boxed-region-ptr's outer box modified (!)";
         LOG(INFO) << dm::DbgInfo::GetInfoString(boxedRegionPtrField);

         Region3BoxedPtr fieldValue = boxedRegionPtrField.Get();
         if (fieldValue == nullptr) {
            updateCb(csg::Region3());
         } else {
            g->region = fieldValue->TraceObjectChanges(reason, std::bind(fieldValueChangedCb, Region3BoxedRef(fieldValue)));
            fieldValueChangedCb(fieldValue); // xxx: all these manual callbacks need to go!  ug!!
         }
      }
   };

   result->boxed = boxedRegionPtrField.TraceObjectChanges(reason, fieldChangedCb);
   fieldChangedCb(); // xxx: all these manual callbacks need to go!  ug!!
   return result;
}

DeepRegionGuardPtr om::DeepTraceRegionVoid(Region3BoxedPtrBoxed const& boxedRegionPtrField,
                                                      const char* reason,
                                                      std::function<void()> updateCb)
{
   return DeepTraceRegion(boxedRegionPtrField, reason, [=](csg::Region3 const& r) {
      updateCb();
   });
}

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
