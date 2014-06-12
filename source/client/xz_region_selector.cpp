#include "pch.h"
#include "xz_region_selector.h"
#include "om/components/terrain.ridl.h"
#include "client/renderer/raycast_result.h"
#include "client/client.h"
#include "client/renderer/renderer.h" // xxx: move to Client

using namespace radiant;
using namespace radiant::client;

XZRegionSelector::XZRegionSelector(om::TerrainPtr terrain, int userFlags) :   
   _inputHandlerId(0),
   _terrain(terrain),
   _userFlags(userFlags)
{
}

XZRegionSelector::~XZRegionSelector()
{
   Deactivate();
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
   RaycastResult castResult = Renderer::GetInstance().QuerySceneRay(x, y, _userFlags);
   if (castResult.GetNumResults() == 0) {
      return false;
   }

   // Compare the very first intersection with the terrain.
   RaycastResult::Result r = castResult.GetResult(0);
   om::EntityPtr entity = r.entity.lock();
   if (!entity || entity->GetObjectId() != _terrain->GetEntity().GetObjectId()) {
      return false;
   }

   // add in the normal to get the adjacent brick
   pt = r.brick + csg::ToInt(r.normal);

   return true;
}

bool XZRegionSelector::IsValidLocation(int x, int y, int z) 
{
   // If we're only selecting the terrain and therefore ignoring entities (done when
   // harvesting, for example), then simply return true.
   if (_userFlags == 1) {
      return true;
   }

   // Otherwise, consult the octree for standability.
   phys::OctTree& octtree = Client::GetInstance().GetOctTree();
   return octtree.GetNavGrid().IsStandable(csg::Point3(x, y, z));
}

void XZRegionSelector::ValidateP1(int newx, int newz)
{
   int dx = (newx >= _p0.x) ? 1 : -1;
   int dz = (newz >= _p0.z) ? 1 : -1;
   int validx, validz;

   // grow in the x direction
   for (validx = _p0.x + dx; validx != newx + dx; validx += dx) {
      for (int z = _p0.z; z != newz + dz; z += dz) {
         if (!IsValidLocation(validx, _p0.y, z)) {
            goto z_check;
         }
      }
   }

z_check:
   validx -= dx;
   // grow in the z direction
   for (validz = _p0.z + dz; validz != newz + dz; validz += dz) {
      for (int x = _p0.x; x != validx + dx; x += dx) {
         if (!IsValidLocation(x, _p0.y, validz)) {
            goto finished;
         }
      }
   }

finished:
   validz -= dz;

   _p1.x = validx;
   _p1.y = _p0.y;
   _p1.z = validz;
}
