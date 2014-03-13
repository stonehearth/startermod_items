#include "pch.h"
#include "xz_region_selector.h"
#include "om/components/terrain.ridl.h"
#include "om/selection.h"
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
   ASSERT(_inputHandlerId == 0);
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

   auto self = shared_from_this();
   _inputHandlerId = Client::GetInstance().AddInputHandler([=](Input const& input) -> bool {
      return self->onInputEvent(input);
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
         if (_finished) {
            deferred_->Resolve(csg::Cube3::Construct(_p0, _p1));
            deferred_ = nullptr;
            Deactivate();
         } else {
            deferred_->Notify(csg::Cube3::Construct(_p0, _p1));
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
   om::Selection s;

   Renderer::GetInstance().QuerySceneRay(x, y, s, _userFlags);
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

#define OK(x, y, z) octtree.CanStandOn(nullptr, csg::Point3(x, y, z))

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
   _p1.y = _p0.y + 1;
   _p1.z = validz;
}
