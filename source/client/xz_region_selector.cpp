#include "pch.h"
#include "xz_region_selector.h"
#include "om/components/terrain.ridl.h"
#include "client/renderer/raycast_result.h"
#include "client/client.h"
#include "client/renderer/renderer.h" // xxx: move to Client

using namespace radiant;
using namespace radiant::client;

XZRegionSelector::XZRegionSelector(om::TerrainPtr terrain) :   
   _inputHandlerId(0),
   _terrain(terrain),
   _requireSupported(false),
   _requireUnblocked(false)
{
}

XZRegionSelector::~XZRegionSelector()
{
   Deactivate();
}

std::shared_ptr<XZRegionSelector> XZRegionSelector::RequireSupported(bool requireSupported)
{
   _requireSupported = requireSupported;
   return shared_from_this();
}

std::shared_ptr<XZRegionSelector> XZRegionSelector::RequireUnblocked(bool requireUnblocked)
{
   _requireUnblocked = requireUnblocked;
   return shared_from_this();
}

std::shared_ptr<XZRegionSelector::Deferred> XZRegionSelector::Activate()
{
   ASSERT(_inputHandlerId == 0);

   _startedP0 = false;
   _finishedP0 = false;
   _finished = false;
   deferred_ = std::make_shared<Deferred>();

   csg::Point2 pt = Renderer::GetInstance().GetMousePosition();
   GetHoverBrick(pt.x, pt.y, _p0);

   _inputHandlerId = Client::GetInstance().AddInputHandler([=](Input const& input) -> bool {
      // keep ourselves alive for as long as the call lasts...
      auto self = shared_from_this();
      return onInputEvent(input);
   });
   
   return deferred_;
}

void XZRegionSelector::Deactivate()
{
   if (_inputHandlerId) {
      Client::GetInstance().RemoveInputHandler(_inputHandlerId);
      _inputHandlerId = 0;
   }
   if (deferred_) {
      deferred_->Reject("tool deactivated");
      deferred_ = nullptr;
   }
}

bool XZRegionSelector::onInputEvent(Input const& evt)
{
   ASSERT(!_finished);
   if (evt.type == Input::MOUSE) {   
      if (!_finishedP0) {
         SelectP0(evt.mouse);
      } else {
         SelectP1(evt.mouse);
      }

      if (_startedP0) {
         csg::Cube3 selectedCube = CreateSelectedCube();

         if (_finished) {
            deferred_->Resolve(selectedCube);
            deferred_ = nullptr;
            Deactivate();
         } else {
            deferred_->Notify(selectedCube);
         }
      }

      if (evt.mouse.up[1] && !evt.mouse.dragging) { // right mouse up
         deferred_->Reject("mouse 2 pressed");
         Deactivate();            
      }

      if (evt.mouse.dragging && evt.mouse.down[1]) {
         // let right mouse button input through so the camera can rotate
         return false;
      }

      return true;

   } else if (evt.type == Input::KEYBOARD) {
      if (evt.keyboard.down && evt.keyboard.key == 256) { // esc
         deferred_->Reject("escape key pressed");
      }
   }
   return false;
}

csg::Cube3 XZRegionSelector::CreateSelectedCube()
{
   csg::Point3 min = _p0;
   csg::Point3 max = _p1;

   // find the min and max points
   for (int i = 0; i < 3; i++) {
      if (min[i] > max[i]) {
         std::swap(min[i], max[i]);
      }
   }

   // the max point is a valid voxel index in the selected region
   // add 1 to include this voxel in the region
   for (int i = 0; i < 3; i++) {
      max[i]++;
   }

   return csg::Cube3(min, max);
}

void XZRegionSelector::SelectP0(const MouseInput &me)
{
   if (GetHoverBrick(me.x, me.y, _p0)) {
      _startedP0 = true;
      ValidateP1(_p0.x, _p0.z);

      if (me.down[0]) {
         _finishedP0 = true;
      }
   }
}

void XZRegionSelector::SelectP1(const MouseInput &me)
{
   csg::Point3 p;
   if (GetHoverBrick(me.x, me.y, p)) {
      ValidateP1(p.x, p.z);
      if (me.up[0]) {
         _finished = true;
      }
   }
}

bool XZRegionSelector::GetHoverBrick(int x, int y, csg::Point3 &pt)
{
   int const terrain_nodes_only_flag = 1;
   RaycastResult castResult = Renderer::GetInstance().QuerySceneRay(x, y, terrain_nodes_only_flag);
   if (castResult.GetNumResults() == 0) {
      return false;
   }

   // Compare the very first intersection with the terrain.
   RaycastResult::Result r = castResult.GetResult(0);
   om::EntityPtr entity = r.entity.lock();
   if (!entity || entity->GetObjectId() != _terrain->GetEntity().GetObjectId()) {
      return false;
   }
   
   // Add in the normal to get the adjacent brick; validate.
   csg::Point3 tempPt = r.brick + csg::ToInt(r.normal);
   if (IsValidLocation(tempPt.x, tempPt.y, tempPt.z)) {
      pt = tempPt;
      return true;
   }
   return false;
}

bool XZRegionSelector::IsValidLocation(int x, int y, int z) const
{
   phys::NavGrid& navGrid = Client::GetInstance().GetOctTree().GetNavGrid();
   csg::Point3 point(x, y, z);

   if (_requireUnblocked && navGrid.IsBlocked(point)) {
      return false;
   }

   if (_requireSupported && !navGrid.IsSupported(point)) {
      return false;
   }

   return true;
}

// Trace lines in the direction of the 'j' vector, while moving across the region in the direction of the 'i' vector.  If we encounter a blockage in the 'j'
// direction, make that obstruction our new 'end' for the j taces--UNLESS the resulting area of the new rectangle is too small, in which case terminate
// immediately.  If the start of a line trace is ever invalid, immediately end.
void XZRegionSelector::largestBox(int di, int dj, int istart, int *iend, int jstart, int *jend, int y, std::function<bool(int x, int y, int z)> const& isValid) const
{
   int lastArea = 0;
   for (int i = istart; i != *iend + di; i += di) {
      if (!isValid(i, y, jstart)) {
         *iend = i - di;
         return;
      }
      for (int j = jstart; j != *jend + dj; j += dj) {
         if (!isValid(i, y, j)) {
            int curArea = abs(istart - *iend) * abs(jstart - (j - dj));
            if (curArea < lastArea * 0.50f) {
               *iend = i - di;
               return;
            }
            *jend = j - dj;
            break;
         }
      }
      lastArea = abs(istart - *iend) * abs(jstart - *jend);
   }
}

void XZRegionSelector::ValidateP1(int newx, int newz)
{
   int dx = (newx >= _p0.x) ? 1 : -1;
   int dz = (newz >= _p0.z) ? 1 : -1;
   int xend = newx;
   int zend = newz;
   int lx = abs(_p0.x - newx);
   int lz = abs(_p0.z - newz);

   if (lx >= lz) {
      largestBox(dx, dz, _p0.x, &xend, _p0.z, &zend, _p0.y, [this](int x, int y, int z) -> bool {
         return IsValidLocation(x, y, z);
      });
   } else {
      largestBox(dz, dx, _p0.z, &zend, _p0.x, &xend, _p0.y, [this](int z, int y, int x) -> bool {
         return IsValidLocation(x, y, z);
      });
   }
   _p1.x = xend;
   _p1.y = _p0.y;
   _p1.z = zend;
}

std::ostream& client::operator<<(std::ostream& os, XZRegionSelector const& o)
{
   return os << "[XZRegionSelector ...]";
}
