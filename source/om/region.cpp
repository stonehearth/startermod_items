#include "pch.h"
#include "region.h"

using namespace ::radiant;
using namespace ::radiant::om;

// xxx: hmm.  guards are looking more like promises!!! (shared_ptr<Promise> in fact).
BoxedRegionGuardPtr om::TraceBoxedRegion3PtrField(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField,
                                                  const char* reason,
                                                  std::function<void(csg::Region3 const& r)> updateCb)
{   
   auto fieldValueChangedCb = [=, &boxedRegionPtrField](BoxedRegion3Ref v) {
      BoxedRegion3Ptr value = v.lock();
      if (value) {
         LOG(INFO) << "boxed-boxed-region-ptr's inner box modified (!)";
         LOG(INFO) << dm::DbgInfo::GetInfoString(*value);
         updateCb(value->Get());
      }
   };

   BoxedRegionGuardPtr result = std::make_shared<BoxedRegionGuard>(boxedRegionPtrField.GetStoreId(), boxedRegionPtrField.GetObjectId());
   BoxedRegionGuardRef r = result;
   auto fieldChangedCb = [=, &boxedRegionPtrField]() {
      BoxedRegionGuardPtr g = r.lock();
      if (g) {
         LOG(INFO) << "boxed-boxed-region-ptr's outer box modified (!)";
         LOG(INFO) << dm::DbgInfo::GetInfoString(boxedRegionPtrField);

         BoxedRegion3Ptr fieldValue = boxedRegionPtrField.Get();
         if (fieldValue == nullptr) {
            updateCb(csg::Region3());
         } else {
            g->region = fieldValue->TraceObjectChanges(reason, std::bind(fieldValueChangedCb, BoxedRegion3Ref(fieldValue)));
            fieldValueChangedCb(fieldValue); // xxx: all these manual callbacks need to go!  ug!!
         }
      }
   };

   result->boxed = boxedRegionPtrField.TraceObjectChanges(reason, fieldChangedCb);
   fieldChangedCb(); // xxx: all these manual callbacks need to go!  ug!!
   return result;
}

BoxedRegionGuardPtr om::TraceBoxedRegion3PtrFieldVoid(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField,
                                                      const char* reason,
                                                      std::function<void()> updateCb)
{
   return TraceBoxedRegion3PtrField(boxedRegionPtrField, reason, [=](csg::Region3 const& r) {
      updateCb();
   });
}

BoxedRegion3Promise::BoxedRegion3Promise(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField, const char* reason)
{
   region_guard_ = TraceBoxedRegion3PtrField(boxedRegionPtrField, reason, [=](csg::Region3 const& r) {
      for (auto& cb : changedCbs_) {
         luabind::call_function<void>(cb, r);
      }
   });
}

BoxedRegion3Promise* BoxedRegion3Promise::PushChangedCb(luabind::object cb) {
   changedCbs_.push_back(cb);
   return this;
}
