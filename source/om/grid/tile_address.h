#ifndef _RADIANT_OM_GRID_TILE_ADDRESS_H
#define _RADIANT_OM_GRID_TILE_ADDRESS_H

#include "math3d.h"
#include "om/om.h"

#include "world.pb.h"

#define TILE_MAP_SIZE   1

BEGIN_RADIANT_OM_NAMESPACE

class TileAddress {
   public:
      TileAddress() { }

      TileAddress(int world_x, int world_y, int world_z) { 
         init(world_x, world_y, world_z);
      }

      TileAddress(float world_x, float world_y, float world_z) { 
         init((int)world_x, (int)world_y, (int)world_z);
      }

      TileAddress(const math3d::ipoint3 &world_pt) {
         init(world_pt.x, world_pt.y, world_pt.z);
      }

      TileAddress(const math3d::point3 &world_pt) {  
         init((int)world_pt.x, (int)world_pt.y, (int)world_pt.z);
      }

      operator size_t() const {
         return (((_rootx * 7) + _rootz) * 7 + _rooty) * 7;
      }

      // xxx: nuke this
      operator math3d::point3() const {
         return math3d::point3((float)_rootx, (float)_rooty, (float)_rootz);
      }

      // xxx: nuke this
      operator math3d::ipoint3() const {
         return math3d::ipoint3(_rootx, _rooty, _rootz);
      }

      void SaveValue(protocol::ipoint3* msg) const {
         static_cast<math3d::ipoint3>(*this).SaveValue(msg);
      }

      void LoadValue(const protocol::ipoint3& msg) {
         _rootx = msg.x();
         _rooty = msg.y();
         _rootz = msg.z();
      }

      bool operator==(const TileAddress &rhs) const {
         return _rootx == rhs._rootx &&
                _rooty == rhs._rooty &&
                _rootz == rhs._rootz;
      }

      bool operator!=(const TileAddress &rhs) const {
         return _rootx != rhs._rootx ||
                _rooty != rhs._rooty ||
                _rootz != rhs._rootz;
      }

      bool operator<(const TileAddress &rhs) const {
         if (_rootx != rhs._rootx) {
            return _rootx < rhs._rootx;
         }
         if (_rooty != rhs._rooty) {
            return _rooty < rhs._rooty;
         }
         return _rootz < rhs._rootz;
      }

      math3d::ipoint3 brick_address(int x, int y, int z) const {
         x -= _rootx;
         y -= _rooty;
         z -= _rootz;
         ASSERT(x >= 0 && x < 16 && y >= 0 && y < 16 && z >= 0 && z < 16);
         return math3d::ipoint3(x, y, z);
      }

      math3d::ipoint3 brick_address(const math3d::ipoint3 &world_pt) const {
         return brick_address(world_pt.x, world_pt.y, world_pt.z);
      }

      math3d::ipoint3 brick_address(const math3d::point3 &world_pt) const {
         return brick_address((int)world_pt.x, (int)world_pt.y, (int)world_pt.z);
      }

      math3d::ipoint3 brickOffset(const math3d::ipoint3 &world_pt) const {
         // warning: unsafe to pass directly to a tile!  can generate oob indicies.
         return math3d::ipoint3(world_pt.x  - _rootx, world_pt.y - _rooty, world_pt.z - _rootz);
      }

      math3d::ipoint3 origin() const { return math3d::ipoint3(_rootx, _rooty, _rootz); }


      static void forEachAddress(const math3d::ibounds3 &bounds, std::function<void (TileAddress &)> fn) {                 
         TileAddress first(bounds._min);
         TileAddress last(bounds._max);
         TileAddress i(first);
         for (i._rootx = first._rootx; i._rootx <= last._rootx; i._rootx += 16) {
            for (i._rootz = first._rootz; i._rootz <= last._rootz; i._rootz += 16) {
               for (i._rooty = first._rooty; i._rooty <= last._rooty; i._rooty += 16) {
                  fn(i);
               }
            }
         }
      }

      struct Hash { 
         size_t operator()(const TileAddress& o) const {
            return std::hash<int>()(o._rootx) ^ std::hash<int>()(o._rooty) ^ std::hash<int>()(o._rootz); 
         }
      };

   protected:
      int init(int component) {
         if (component >= 0) {
            return component & ~0x0f; // floor
         }
         int flipped = (-component + 15) & ~0x0f;
         return -flipped;
      }
      void init(int world_x, int world_y, int world_z) { 
         _rootx = init(world_x);
         _rooty = init(world_y);
         _rootz = init(world_z);
      }

      void init(float world_x, float world_y, float world_z) { 
         ASSERT(false); // why!?
         _rootx = ((int)world_x) & ~0x0f;
         _rooty = ((int)world_y) & ~0x0f;
         _rootz = ((int)world_z) & ~0x0f;
      }

   protected:
      int _rootx;
      int _rooty;
      int _rootz;
};

std::ostream& operator<<(std::ostream& out, const TileAddress& addr);

END_RADIANT_OM_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::om::TileAddress, Protocol::ipoint3)


#endif // _RADIANT_OM_GRID_TILE_ADDRESS_H

