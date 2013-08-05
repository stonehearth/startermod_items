#ifndef _RADIANT_OM_RENDER_INFO_H
#define _RADIANT_OM_RENDER_INFO_H

#include "om/om.h"
#include "om/object_enums.h"
#include "dm/set.h"
#include "dm/record.h"
#include "store.pb.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class RenderInfo : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(RenderInfo, render_info);

public:
   //std::string GetKind() const { return *kind_; }
   //void SetKind(std::string kind) { kind_ = kind; }

   void SetDisplayIconic(bool value) { iconic_ = value; }
   bool GetDisplayIconic() { return *iconic_; }

private:
   NO_COPY_CONSTRUCTOR(RenderInfo)

   void InitializeRecordFields() override;

public:
   dm::Boxed<bool>            iconic_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_RENDER_INFO_H
