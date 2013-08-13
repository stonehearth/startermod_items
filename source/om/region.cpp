#include "pch.h"
#include "region.h"

using namespace ::radiant;
using namespace ::radiant::om;

dm::Guard om::TraceBoxedRegion3PtrField(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField,
                                        const char* reason,
                                        std::function<void(csg::Region3 const& r)> updateCb)
{   
   auto fieldValueChangedCb = [=](BoxedRegion3Ref v) {
      BoxedRegion3Ptr value = v.lock();
      if (value) {
         updateCb(value->Get());
      }
   };

   dm::Guard g; // xxx: this needs to be a new guard for every function invocation.  Is that the case??
   auto fieldChangedCb = [=, &g, &boxedRegionPtrField]() {
      BoxedRegion3Ptr fieldValue = boxedRegionPtrField.Get();
      if (fieldValue == nullptr) {
         updateCb(csg::Region3());
      } else {
         LOG(WARNING) << "----------> Creating new guard " << g.GetId();
         g = fieldValue->TraceObjectChanges(reason, std::bind(fieldValueChangedCb, BoxedRegion3Ref(fieldValue)));
      }
   };

   return boxedRegionPtrField.TraceObjectChanges(reason, fieldChangedCb);
}

BoxedRegion3Promise::BoxedRegion3Promise(dm::Boxed<BoxedRegion3Ptr> const& boxedRegionPtrField, const char* reason) {
   guard_ = TraceBoxedRegion3PtrField(boxedRegionPtrField, reason, [=](csg::Region3 const& r) {
      for (auto& cb : changedCbs_) {
         luabind::call_function<void>(cb, r);
      }
   });
}

BoxedRegion3Promise* BoxedRegion3Promise::PushChangedCb(luabind::object cb) {
   changedCbs_.push_back(cb);
   return this;
}
