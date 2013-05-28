#ifndef _RADIANT_OM_RENDER_RIG_H
#define _RADIANT_OM_RENDER_RIG_H

#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/set.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class RenderRig : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(RenderRig);
   void Construct(json::ConstJsonObject const& obj) override;

private:
   dm::Set<std::string>    rigs_;
   dm::Boxed<std::string>  animationTable_;
   dm::Boxed<float>        scale_;

public:
   const decltype(rigs_)& GetRigs() const { return rigs_; }

   void AddRig(std::string rig) { rigs_.Insert(rig); }
   void RemoveRig(std::string rig) { rigs_.Remove(rig); }

   void SetScale(float scale) { scale_ = scale; }
   float GetScale() const { return *scale_; }

   void SetAnimationTable(std::string table) { animationTable_ = table; }
   std::string GetAnimationTable() const { return *animationTable_; } 


private:
   NO_COPY_CONSTRUCTOR(RenderRig)

   void InitializeRecordFields() override;
};

class RenderRigIconic : public RenderRig
{
public:
   DEFINE_OM_OBJECT_TYPE(RenderRigIconic);
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_RENDER_RIG_H
