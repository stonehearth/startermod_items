#ifndef _RADIANT_OM_REGION_H
#define _RADIANT_OM_REGION_H

#include "math3d.h"
#include "dm/record.h"
#include "csg/region.h"
#include "om/all_object_types.h"

BEGIN_RADIANT_OM_NAMESPACE

class Region : public dm::Object
{
public:
   DEFINE_OM_OBJECT_TYPE(Region);

   csg::Region3& Modify() {
      MarkChanged();
      return region_;
   }

   const csg::Region3& operator*() const {
      return region_;
   }

   dm::Guard Trace(const char* reason, std::function<void(const csg::Region3&)> cb) const {
      return TraceObjectChanges(reason, [=]() {
         cb(region_);
      });
   }

   std::ostream& Log(std::ostream& os, std::string indent) const override {
      os << "region [oid:" << GetObjectId() << " value:" << dm::Format<csg::Region3>(region_, indent) << "]" << std::endl;
      return os;
   }

private:
   void CloneObject(Object* c, dm::CloneMapping& mapping) const override {
      Region& copy = static_cast<Region&>(*c);
      mapping.objects[GetObjectId()] = copy.GetObjectId();
      copy.region_ = region_;
   }

protected:
   void SaveValue(const dm::Store& store, Protocol::Value* msg) const override {
      dm::SaveImpl<csg::Region3>::SaveValue(store, msg, region_);
   }
   void LoadValue(const dm::Store& store, const Protocol::Value& msg) override {
      dm::SaveImpl<csg::Region3>::LoadValue(store, msg, region_);
   }

private:
   csg::Region3    region_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
