#ifndef _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H
#define _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H

#include "namespace.h"
#include "core/deferred.h"
#include "radiant_file.h"
#include "om/components/terrain.ridl.h"
#include "csg/point.h"
#include "csg/color.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class XZRegionSelector : public std::enable_shared_from_this<XZRegionSelector>
{
   public:
      class Deferred : public core::Deferred<csg::Cube3, std::string>
      {
      };
      DECLARE_SHARED_POINTER_TYPES(Deferred)

   public:
      XZRegionSelector(om::TerrainPtr terrain, int userFlags);
      ~XZRegionSelector();

      std::shared_ptr<Deferred> Activate();
      void Deactivate();

   protected:
      bool onInputEvent(Input const& evt);
      void SelectP0(const MouseInput &me);
      void SelectP1(const MouseInput &me);

      bool GetHoverBrick(int x, int y, csg::Point3 &pt);
      void ValidateP1(int x, int z);

   protected:
      std::shared_ptr<Deferred>        deferred_;
      int                              _inputHandlerId;
      om::TerrainPtr                   _terrain;
      csg::Color3                   _color;
      csg::Point3                  _p0;
      csg::Point3                  _p1;
      bool                             _finishedP0;
      bool                             _startedP0;
      bool                             _finished;
      int                          _userFlags;
};


END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H
 