#ifndef _RADIANT_OM_RENDER_GRID_H
#define _RADIANT_OM_RENDER_GRID_H

#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/boxed.h"
#include "dm/record.h"
#include "om/grid/grid.h"
#include "component.h"

BEGIN_RADIANT_OM_NAMESPACE

class RenderGrid : public Component
{
public:
   DEFINE_OM_OBJECT_TYPE(RenderGrid);

   std::shared_ptr<Grid> GetGrid() const { return (*grid_).lock(); }
   void SetGrid(om::GridPtr grid) { grid_ = grid; }

private:
   NO_COPY_CONSTRUCTOR(RenderGrid)

   void InitializeRecordFields() override {
      Component::InitializeRecordFields();
      AddRecordField("grid", grid_);
   }

public:
   dm::Boxed<om::GridRef>   grid_;
};

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_RENDER_GRID_H
