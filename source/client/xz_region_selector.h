#ifndef _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H
#define _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H

#include "namespace.h"
#include "core/deferred.h"
#include "radiant_file.h"
#include "om/components/terrain.h"
#include "math3d.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class XZRegionSelector : public std::enable_shared_from_this<XZRegionSelector>
{
   public:
      class Deferred : public core::Deferred2<math3d::ipoint3, math3d::ipoint3>
      {
      };

   public:
      XZRegionSelector(om::TerrainPtr terrain);
      ~XZRegionSelector();

      std::shared_ptr<Deferred> Activate();

   protected:
      void Deactivate();
      void onInputEvent(const MouseEvent &evt, bool &handled, bool &uninstall);
      void SelectP0(const MouseEvent &me);
      void SelectP1(const MouseEvent &me);

      bool GetHoverBrick(int x, int y, math3d::ipoint3 &pt);
      void ValidateP1(int x, int z);
      void GetDrawBox(math3d::aabb &box);

   protected:
      std::shared_ptr<Deferred>        deferred_;
      int                              _inputHandlerId;
      om::TerrainPtr                   _terrain;
      math3d::color3                   _color;
      math3d::ipoint3                  _p0;
      math3d::ipoint3                  _p1;
      bool                             _finishedP0;
      bool                             _startedP0;
      bool                             _finished;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H
 