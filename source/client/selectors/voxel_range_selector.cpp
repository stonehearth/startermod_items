#include "pch.h"
#include "voxel_range_selector.h"
#include "client/client.h"
#include "client/renderer/renderer.h"
#include "Horde3DRadiant.h"
#include "om/components/terrain.h"
#include "om/grid/grid.h"
#include "om/entity.h"

using namespace radiant;
using namespace radiant::client;

VoxelRangeSelector::VoxelRangeSelector(om::TerrainPtr terrain) :   
   _inputHandlerId(0),
   _modified(true),
   _terrain(terrain),
   _state(SELECT_P0),
   _color(math3d::color3::red)
{
   SetDimensions(2);
   _shape = h3dRadiantAddDebugShapes(H3DRootNode, "voxel range selector");
   LOG(WARNING) << "creating shape for voxel range selector " << _shape;
}

VoxelRangeSelector::VoxelRangeSelector(om::TerrainPtr terrain, Selector::SelectionFn fn) :   
   _inputHandlerId(0),
   _modified(true),
   _terrain(terrain),
   _state(SELECT_P0),
   _color(math3d::color3::red)
{
   SetDimensions(2);
   RegisterSelectionFn(fn);

   _shape = h3dRadiantAddDebugShapes(H3DRootNode, "voxel range selector");
   LOG(WARNING) << "creating shape for voxel range selector " << _shape;
}

VoxelRangeSelector::~VoxelRangeSelector()
{
   ASSERT(_inputHandlerId == 0);
}

void VoxelRangeSelector::SetColor(const math3d::color3 &color)
{
   _color = color;
   _modified = true;
}


void VoxelRangeSelector::SetDimensions(int dimensions)
{
   ASSERT(_state == SELECT_P0);

   switch (dimensions) {
   case 0:
      _finished = SELECT_P1;
      break;

   default:
      _finished = SELECT_P2;
      break;
   }

   ASSERT(_finished <= NUM_STATES);

   _modified = true;
}


void VoxelRangeSelector::Prepare()
{
   if (_modified) {
      math3d::aabb box;
      GetDrawBox(box);

      LOG(WARNING) << "updating shape for voxel range selector " << _shape << " " << box;
      h3dRadiantClearDebugShape(_shape);
      h3dRadiantAddDebugBox(_shape, box, math3d::color4(_color.r, _color.b, _color.g, 255));
      h3dRadiantCommitDebugShape(_shape);
      _modified = false;
   }
}

void VoxelRangeSelector::RegisterSelectionFn(Selector::SelectionFn fn)
{
   _selectionFn = fn;
}

void VoxelRangeSelector::Activate()
{
   _modified = true;
   _p0.set_zero();
   _p1.set_zero();
   _p2.set_zero();
   _state = SELECT_P0;

   csg::Point2 pt = Renderer::GetInstance().GetMousePosition();
   GetHoverBrick(pt.x, pt.y, _p0);

   traces_ += Renderer::GetInstance().TraceFrameStart([=]() { Prepare(); });

   auto self = shared_from_this();
   _inputHandlerId = Renderer::GetInstance().SetMouseInputCallback([=](const MouseEvent &evt, bool &handled, bool &uninstall) {
      self->onInputEvent(evt, handled, uninstall);
   });
   
}

void VoxelRangeSelector::Deactivate()
{
   if (_inputHandlerId) {
      h3dRemoveNode(_shape);
      Renderer::GetInstance().RemoveInputEventHandler(_inputHandlerId);
      _inputHandlerId = 0;
      _shape = 0;
   }
   _selectionFn = nullptr;
   feedbackFn_ = nullptr;
}

void VoxelRangeSelector::onInputEvent(const MouseEvent &evt, bool &handled, bool &uninstall)
{
   typedef bool (VoxelRangeSelector::*DispatchFn)(const MouseEvent &me);
   static const DispatchFn dispatch[] = {
      &VoxelRangeSelector::SelectP0,
      &VoxelRangeSelector::SelectP1,
      &VoxelRangeSelector::SelectP2,
   };

   if (_state <= _finished) {
      handled = (this->*dispatch[_state])(evt);
      if (_state > SELECT_P0 && feedbackFn_) {
         math3d::ibounds3 b;
         for (int i = 0; i < 3; i++) {
            b._min[i] = std::min(_p0[i], _p1[i]);
            b._max[i] = std::max(_p0[i], _p1[i]) + 1;
         }
         feedbackFn_(b);
      }
   }
}

bool VoxelRangeSelector::SelectP0(const MouseEvent &me)
{
   // LOG(WARNING) << "P0... " << me.x << ", " << me.y;
   if (GetHoverBrick(me.x, me.y, _p0)) {
      LOG(WARNING) << "hover brick is " << _p0;
      _modified = true;

      if (me.down[0]) {
         _state = SELECT_P1;
         if (_state == _finished) {
            om::Selection s;
            s.SetBounds(math3d::ibounds3(_p0, _p0 + math3d::ipoint3(1, 1, 1)));
            if (_selectionFn) {
               _selectionFn(s);
               Deactivate();
            }
            return false;
         } else {
            ValidateP1(_p0.x, _p0.z);
         }
      }
      return true;
   }
   return false;
}

bool VoxelRangeSelector::SelectP1(const MouseEvent &me)
{
   //LOG(WARNING) << "P1...";
   math3d::ipoint3 p;
   if (GetHoverBrick(me.x, me.y, p)) {

      _modified = true;
      ValidateP1(p.x, p.z);
      
      if (me.up[0]) {
         _state = SELECT_P2;

         math3d::ibounds3 b;
         for (int i = 0; i < 3; i++) {
            b._min[i] = std::min(_p0[i], _p1[i]);
            b._max[i] = std::max(_p0[i], _p1[i]) + 1;
         }
         om::Selection s;
         s.SetBounds(b);

         if (_selectionFn) {
            _selectionFn(s);
            Deactivate();
         }
      }
      return true;
   }
   return false;
}

bool VoxelRangeSelector::SelectP2(const MouseEvent &me)
{
   return false;
}
 
bool VoxelRangeSelector::GetHoverBrick(int x, int y, math3d::ipoint3 &pt)
{
   om::Selection s;

   Renderer::GetInstance().QuerySceneRay(x, y, s);
   if (!s.HasBlock()) {
      return false;
   }
   if (s.HasEntities() && (s.GetEntities().front() != _terrain->GetEntity().GetEntityId())) {
      return false;
   }

   // add in the normal to get the adjacent brick
   pt = math3d::ipoint3(math3d::point3(s.GetBlock()) + s.GetNormal());

   return true;
}


void VoxelRangeSelector::ValidateP1(int newx, int newz)
{
   int dx = (newx >= _p0.x) ? 1 : -1;
   int dz = (newz >= _p0.z) ? 1 : -1;
   int validx, validz;
   auto const& octtree = Client::GetInstance().GetOctTree();

#define OK(x, y, z) octtree.CanStand(math3d::ipoint3(x, y, z))

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

void VoxelRangeSelector::GetDrawBox(math3d::aabb &box)
{
   box = math3d::aabb(math3d::point3(_p0), math3d::point3(_p0));

   switch (_state) {
   case SELECT_P1:
   case SELECT_P2:
      box.add_point(math3d::point3(_p1));
      break;
   }

   box._max += math3d::point3(0.5, 0.0, 0.5);
   box._min -= math3d::point3(0.5, 0.0, 0.5);
}
