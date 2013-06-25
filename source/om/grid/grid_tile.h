#ifndef _RADIANT_OM_GRID_TILE_H
#define _RADIANT_OM_GRID_TILE_H

#include "math3d.h"
#include "om/om.h"
#include "om/all_object_types.h"
#include "dm/record.h"
#include "dm/boxed.h"
#include "dm/store.h"
#include "iterator.h"
#include "radiant.pb.h"

#define BOX_SIZE     16

BEGIN_RADIANT_OM_NAMESPACE

class GridTile : public dm::Record {
public:
   DEFINE_OM_OBJECT_TYPE(GridTile, grid_tile);

   void InitializeRecordFields() override;

   const math3d::ipoint3 getLocation() const;
   void SetGrid(std::shared_ptr<Grid> grid);
   void SetLocation(const math3d::ipoint3 &pt);

   bool Intersect(const math3d::ray3& ray, math3d::point3& pt, math3d::point3& normal, float &t /* in-out */);

   GridPtr GetGrid();

   unsigned char getVoxel(int x, int y, int z) const;
   unsigned char getVoxel(const math3d::ipoint3 &offset) const { return getVoxel(offset.x, offset.y, offset.z); }

   bool isEmpty() const { return cellCount_ == 0; }
   bool isEmpty(int x, int y, int z) const;
   bool isEmpty(const math3d::ipoint3 &offset) const { return getVoxel(offset) == 0; }

   void clear();

   void setVoxel(int x, int y, int z, unsigned char value);
   void setVoxel(const math3d::ipoint3 &offset, unsigned char value) { setVoxel(offset.x, offset.y, offset.z, value); }

   inline bool contains(const math3d::ipoint3 &pt) const {
      return contains(pt.x, pt.y, pt.z);
   }
   inline bool contains(int x, int y, int z) const { 
      return x >= 0 && x < BOX_SIZE && 
             y >= 0 && y < BOX_SIZE && 
             z >= 0 && z < BOX_SIZE;
   }

   void zero();

   std::string compress(unsigned char *src, int len) const;
   bool decompress(std::string compressed, uchar *dst, int len);

   TileIterator iterator() const;
   TileIterator iterator(const math3d::ibounds3 &bounds) const;

   static const int size;

private:
   void SaveValue(const dm::Store& store, Protocol::Value* msg) const override;
   void LoadValue(const dm::Store& store, const Protocol::Value& msg) override;
   void CloneObject(dm::Object* copy, dm::CloneMapping& mapping) const override;

protected:
   dm::Boxed<std::weak_ptr<Grid>>   _grid;
   dm::Boxed<math3d::ipoint3>       _location;
   dm::Boxed<int>                   cellCount_;

   // xxx: need special logic to mvoe these bad boys.  Make sure
   // you don't screw up dm::Record remoting when you send them
   unsigned char                 _cells[BOX_SIZE][BOX_SIZE][BOX_SIZE]; // y, z, x
};

END_RADIANT_OM_NAMESPACE

#endif // _RADIANT_OM_GRID_TILE_H
