#include "pch.h"
#include "xz_region_selector.h"
#include "om/components/terrain.h"
#include "om/selection.h"
#include "client/client.h"
#include "client/renderer/renderer.h"

using namespace radiant;
using namespace radiant::client;

XZRegionSelector::XZRegionSelector(om::TerrainPtr terrain) :   
   _inputHandlerId(0),
   _terrain(terrain)
{
}

XZRegionSelector::~XZRegionSelector()
{
   ASSERT(_inputHandlerId == 0);
}

std::shared_ptr<XZRegionSelector::Deferred> XZRegionSelector::Activate()
{
   _startedP0 = false;
   _finishedP0 = false;
   _finished = false;
   deferred_ = std::make_shared<Deferred>();

   csg::Point2 pt = Renderer::GetInstance().GetMousePosition();
   GetHoverBrick(pt.x, pt.y, _p0);

   auto self = shared_from_this();
   _inputHandlerId = Renderer::GetInstance().SetMouseInputCallback([=](const MouseEvent &windowMouse, const MouseEvent &browserMouse, bool &handled, bool &uninstall) {
      self->onInputEvent(windowMouse, handled, uninstall);
   });
   
   return deferred_;
}

void XZRegionSelector::Deactivate()
{
   if (_inputHandlerId) {
      Renderer::GetInstance().RemoveInputEventHandler(_inputHandlerId);
      _inputHandlerId = 0;
   }
   deferred_ = nullptr;
}

void XZRegionSelector::onInputEvent(const MouseEvent &evt, bool &handled, bool &uninstall)
{
   ASSERT(!_finished);
   
   if (!_finishedP0) {
      SelectP0(evt);
   } else {
      SelectP1(evt);
   }

   if (_startedP0) {
      csg::Point3 p0, p1;
      for (int i = 0; i < 3; i++) {
         p0[i] = std::min(_p0[i], _p1[i]);
         p1[i] = std::max(_p0[i], _p1[i]);
      }
      if (_finished) {
         deferred_->Resolve(p0, p1);
         Deactivate();
      } else {
         deferred_->Notify(p0, p1);
      }
   }
}

void XZRegionSelector::SelectP0(const MouseEvent &me)
{
   if (GetHoverBrick(me.x, me.y, _p0)) {
      _startedP0 = true;
      ValidateP1(_p0.x, _p0.z);

      if (me.down[0]) {
         _finishedP0 = true;
      }
   }
}

void XZRegionSelector::SelectP1(const MouseEvent &me)
{
   //LOG(WARNING) << "P1...";
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
   om::Selection s;

   Renderer::GetInstance().QuerySceneRay(x, y, s);
   if (!s.HasBlock()) {
      return false;
   }
   if (s.HasEntities() && (s.GetEntities().front() != _terrain->GetEntity().GetObjectId())) {
      return false;
   }

   // add in the normal to get the adjacent brick
   pt = csg::ToInt(csg::ToFloat(s.GetBlock()) + s.GetNormal());

   return true;
}


void XZRegionSelector::ValidateP1(int newx, int newz)
{
   int dx = (newx >= _p0.x) ? 1 : -1;
   int dz = (newz >= _p0.z) ? 1 : -1;
   int validx, validz;
   auto const& octtree = Client::GetInstance().GetOctTree();

#define OK(x, y, z) octtree.CanStand(csg::Point3(x, y, z))

//#define OK(x, y, z) (grid->getVoxelResident(x, y, z) == 0 && grid->getVoxelResident(x, y - 1, z) != 0)

   // grow in the x direction
   for (validx = _p0.x + dx; validx != newx + dx; validx += dx) {
      for (int z = _p0.z; z != newz + dz; z += dz) {
         if (!OK(validx, _p0.y, z)) {
            goto z_check;
         }
      }
   }

z_check:
   validx -= dx;
   // grow in the z direction
   for (validz = _p0.z + dz; validz != newz + dz; validz += dz) {
      for (int x = _p0.x; x != validx + dx; x += dx) {
         if (!OK(x, _p0.y, validz)) {
            goto finished;
         }
      }
   }

finished:
   validz -= dz;

   _p1.x = validx;
   _p1.y = _p0.y;
   _p1.z = validz;

   //LOG(WARNING) << "P1..." << _p1;
}
