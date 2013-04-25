#ifndef _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H
#define _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H

#include "namespace.h"
#include "physics/namespace.h"
#include "client/input_event.h"
#include "om/selection.h"
#include "om/om.h"
#include "selector.h"
#include "Horde3D.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class VoxelRangeSelector : public std::enable_shared_from_this<VoxelRangeSelector>,
                           public Selector {
   public:
      VoxelRangeSelector(om::TerrainPtr terrain);
      VoxelRangeSelector(om::TerrainPtr terrain, Selector::SelectionFn fn);
      virtual ~VoxelRangeSelector();

   public:
      void Deactivate() override;
      void Prepare() override;

      typedef std::function<void(const math3d::ibounds3&)> FeedbackFn;
      void RegisterFeedbackFn(FeedbackFn fn) { feedbackFn_ = fn; }
      void RegisterSelectionFn(Selector::SelectionFn fn) override;

   public:
      void SetColor(const math3d::color3 &color);
      void SetDimensions(int dimensions);

   public:
      void Activate();

   protected:
      enum State {
         SELECT_P0 = 0,
         SELECT_P1 = 1,
         SELECT_P2 = 2,
         NUM_STATES,
      };
      void onInputEvent(const MouseInputEvent &evt, bool &handled, bool &uninstall);

      bool SelectP0(const MouseInputEvent &me);
      bool SelectP1(const MouseInputEvent &me);
      bool SelectP2(const MouseInputEvent &me);

      bool GetHoverBrick(int x, int y, math3d::ipoint3 &pt);
      void ValidateP1(int x, int z);
      void GetDrawBox(math3d::aabb &box);

   protected:
      H3DNode                          node_;
      Selector::SelectionFn            _selectionFn;
      FeedbackFn                       feedbackFn_;
      InputCallbackId                  _inputHandlerId;
      bool                             _modified;
      om::TerrainPtr                   _terrain;

      H3DNode                          _shape;
      State                            _state;
      State                            _finished;
      math3d::color3                   _color;
      math3d::ipoint3                  _p0;
      math3d::ipoint3                  _p1;
      math3d::ipoint3                  _p2;
      dm::GuardSet                       traces_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_VOXEL_RANGE_SELECTOR_H
 