#include "pch.h"
#include "region.h"

using namespace ::radiant;
using namespace ::radiant::csg;

template <class S, int C>
Region<S, C>::Region()
{
   cubes_.reserve(64);
}

template <class S, int C>
Region<S, C>::Region(const Cube& cube)
{
   cubes_.reserve(64);
   cubes_.push_back(cube);
}

template <class S, int C>
S Region<S, C>::GetArea() const
{
   S area = 0;
   for (const auto& cube : cubes_) {
      area += cube.GetArea();
   }
   return area;
}


template <class S, int C>
void Region<S, C>::Clear()
{
   cubes_.clear();
}

template <class S, int C>
void Region<S, C>::Add(const Cube& cube)
{
   Region unique(cube);

   for (const Cube& c : *this) {
      unique.Subtract(c);
   }
   cubes_.insert(cubes_.end(), unique.begin(), unique.end());
}

template <class S, int C>
void Region<S, C>::Add(const Point& point)
{
   Add(Cube(point));
}

template <class S, int C>
void Region<S, C>::AddUnique(const Cube& cube)
{
   ASSERT(!Intersects(cube));
   cubes_.push_back(cube);
}

template <class S, int C>
void Region<S, C>::AddUnique(const Region& region)
{
   for (Cube const& cube : region) {
      ASSERT(!Intersects(cube));
      cubes_.push_back(cube);
   }
}

template <class S, int C>
void Region<S, C>::Subtract(const Cube& cube)
{
   CubeVector added;

   unsigned int i = 0;
   unsigned int size = cubes_.size();

   while (i < size) {
      Region replacement = cubes_[i] - cube;
      if (replacement.IsEmpty()) {
         cubes_[i] = cubes_[size - 1];
         size--;
      } else {
         cubes_[i] = replacement[0];
         added.insert(added.end(), replacement.begin() + 1, replacement.end());
         i++;
      }
   }
   cubes_.resize(size);
   cubes_.insert(cubes_.end(), added.begin(), added.end());
}

template <class S, int C>
Region<S, C> Region<S, C>::operator-(const Cube& cube) const
{
   Region result(*this);
   result -= cube;
   return result;
}

template <class S, int C>
Region<S, C> Region<S, C>::operator-(const Region& r) const
{
   Region result(*this);
   for (const auto& c : r) {
      result -= c;
   }
   return result;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator-=(const Cube& cube)
{
   Subtract(cube);
   return *this;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator+=(const Cube& cube)
{
   Add(cube);
   return *this;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator&=(const Cube& cube)
{
   unsigned int i = 0;
   unsigned int size = cubes_.size();

   while (i < size) {
      Cube replacement = cubes_[i] & cube;
      if (replacement.IsEmpty()) {
         cubes_[i] = cubes_[size - 1];
         size--;
      } else {
         cubes_[i] = replacement;
         i++;
      }
   }
   cubes_.resize(size);
   return *this;
}


template <class S, int C>
const Region<S, C>& Region<S, C>::operator&=(const Region& other)
{
   Region result;

   // the union of all our little rects clipped against the other region
   for (Cube const& cube : cubes_) {
      result.AddUnique(cube & other);
   }

   *this = result;
   return *this;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator=(const Region& r)
{
   cubes_.clear();
   (*this) += r;
   return *this;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator-=(const Region& r)
{
   Subtract(r);
   return *this;
}


template <class S, int C>
void Region<S, C>::Subtract(const Region& r)
{
   for (const auto &rc : r) {
      Subtract(rc);
   }
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator+=(const Region& r)
{
   for (const auto &rc : r) {
      Add(rc);
   }
   return *this;
}

template <class S, int C>
bool Region<S, C>::Intersects(const Cube& cube) const
{
   for (const Cube& c : *this) {
      if (cube.Intersects(c)) {
         return true;
      }
   }
   return false;
}
template <class S, int C>
bool Region<S, C>::Contains(const Point& pt) const
{
   for (Cube const& cube : cubes_) {
      if (cube.Contains(pt)) {
         return true;
      }
   }
   return false;
}

template <class S, int C>
Point<S, C> Region<S, C>::GetClosestPoint2(const Point& from, S *dSquared) const
{
   ASSERT(!IsEmpty());

   S best = (S)INT_MAX;

   Point closest;
   for (int i = 0; i < C; i++) {
      // xxx - need some kind of type traits to make this NOT SUCK
      closest[i] = (S)INT_MAX;
   }

   for (const auto &c : cubes_) {
      S d2;
      Point candidate = c.GetClosestPoint2(from, &d2);
      if (d2 < best) {
         best = d2;
         closest = candidate;
      }
   }
   if (dSquared) {
      *dSquared = best;
   }
   return closest;
}

template <class S, int C>
void Region<S, C>::Optimize()
{
}

template <class S, int C>
Cube<S, C> Region<S, C>::GetBounds() const
{
   ASSERT(!IsEmpty());

   Cube bounds = cubes_[0];
   int i, c = cubes_.size();
   for (i = 1; i < c; i++) {
      bounds.Grow(cubes_[i].GetMin());
      bounds.Grow(cubes_[i].GetMax());
   }
   return bounds;
}

template <class S, int C>
Region<S, C> Region<S, C>::ProjectOnto(int axis, S plane) const
{
   Region result;

   for (const auto &c : cubes_) {
      result.Add(c.ProjectOnto(axis, plane));
   }
   return result;
}

template <class S, int C>
void Region<S, C>::Translate(const Point& pt)
{
   for (auto &c : cubes_) {
      c.Translate(pt);
   }
}

template <class S, int C>
static std::shared_ptr<Region<S, C>> CopyRegion(std::shared_ptr<Region<S, C>> other)
{
   return std::make_shared<Region<S, C>>(*other);
}

template <class S, int C>
std::shared_ptr<Region<S, C>> RegionClip(const Region<S, C>& region, const Cube<S, C>& cube)
{
   std::shared_ptr<Region<S, C>> result = std::make_shared<Region<S, C>>(region);
   (*result) &= cube;
   return result;
}

template <class S, int C>
static void AddCube(Region<S, C>& region, typename Region<S, C>::Cube const& cube)
{
   region.Add(cube);
}


template <class S, int C>
static void AddPoint(Region<S, C>& region, typename Region<S, C>::Point const& point)
{
   region.Add(point);
}

template <class S, int C>
static void RemoveCube(Region<S, C>& region, typename Region<S, C>::Cube const& cube)
{
   region -= cube;
}


template <class S, int C>
static void RemovePoint(Region<S, C>& region, typename Region<S, C>::Point const& point)
{
   region -= Cube<S, C>(point);
}

template <class S, int C>
static void AddUniqueCube(Region<S, C>& region, typename Region<S, C>::Cube const& cube)
{
   region.AddUnique(cube);
}

template <class S, int C>
luabind::scope Region<S, C>::RegisterLuaType(struct lua_State* L, const char* name)
{
   using namespace luabind;
   return
      class_<Region<S, C>, std::shared_ptr<Region<S, C>>>(name)
         .def(tostring(const_self))
         .def(constructor<>())
         .def(constructor<const Cube&>())
         .def("copy",               &CopyRegion<S, C>)
         .def("empty",              &Region<S, C>::IsEmpty)
         .def("get_area",           &Region<S, C>::GetArea)
         .def("clear",              &Region<S, C>::Clear)
         .def("get_bounds",         &Region<S, C>::GetBounds)
         .def("optimize",           &Region<S, C>::Optimize)
         .def("intersects",         &Region<S, C>::Intersects)
         .def("add_cube",           &AddCube<S, C>)
         .def("add_point",          &AddPoint<S, C>)
         .def("add_unique",         &AddUniqueCube<S, C>)
         .def("remove_cube",        &RemoveCube<S, C>)
         .def("remove_point",       &RemovePoint<S, C>)
         .def("subtract",           static_cast<void(Region<S, C>::*)(const Cube& other)>(&Region<S, C>::Subtract))
         .def("contents",           &Region<S, C>::GetContents, return_stl_iterator)
         .def("clip",               &RegionClip<S, C>)
         .def("get_num_rects",      &Region<S, C>::GetRectCount)
         .def("get_rect",           &Region<S, C>::GetRect)
         .def("get_closest_point",  &Region<S, C>::GetClosestPoint)
         .def("translate",          &Region<S, C>::Translate)
         .def("contains",           &Region<S, C>::Contains)
      ;
}

#if 0
Region3 radiant::csg::GetBorderXZ(const Region3 &other)
{
   Region3 result;

   for (const Cube3 &c : other) {
      Point3 p0 = c.GetMin();
      Point3 p1 = c.GetMax();
      Region3::ScalarType w = p1[0] - p0[0], h = p1[2] - p0[2];
      result.Add(Cube3(p0 - Point3(0, 0, 1), p0 + Point3(w, 1, 0)));  // top
      result.Add(Cube3(p0 - Point3(1, 0, 0), p0 + Point3(0, 1, h)));  // left
      result.Add(Cube3(p0 + Point3(0, 0, h), p0 + Point3(w, 1, h + 1)));  // bottom
      result.Add(Cube3(p0 + Point3(w, 0, 0), p0 + Point3(w + 1, 1, h)));  // right
   }
   result.Optimize();

   return result;
}
#endif

#define MAKE_REGION(Cls) \
   const Cls Cls::empty; \
   template Cls::Region(); \
   template Cls::Region(const Cls::Cube&); \
   template Cls::ScalarType Cls::GetArea() const; \
   template void Cls::Clear(); \
   template void Cls::Add(const Cls::Cube&); \
   template void Cls::Add(const Cls::Point&); \
   template void Cls::AddUnique(const Cls::Cube&); \
   template void Cls::AddUnique(const Cls&); \
   template void Cls::Subtract(const Cls::Cube&); \
   template void Cls::Subtract(const Cls&); \
   template Cls Cls::operator-(const Cls&) const; \
   template Cls Cls::operator-(const Cls::Cube&) const; \
   template const Cls& Cls::operator&=(const Cls::Cube&); \
   template const Cls& Cls::operator&=(const Cls&); \
   template const Cls& Cls::operator+=(const Cls::Cube&); \
   template const Cls& Cls::operator-=(const Cls::Cube&); \
   template const Cls& Cls::operator=(const Cls&); \
   template const Cls& Cls::operator+=(const Cls&); \
   template const Cls& Cls::operator-=(const Cls&); \
   template bool Cls::Intersects(const Cls::Cube&) const; \
   template bool Cls::Contains(const Cls::Point&) const; \
   template Cls::Point Cls::GetClosestPoint2(const Cls::Point&, Cls::ScalarType*) const; \
   template void Cls::Optimize(); \
   template Cls::Cube Cls::GetBounds() const; \
   template Cls Cls::ProjectOnto(int axis, Cls::ScalarType plane) const; \
   template void Cls::Translate(const Cls::Point& pt); \
   template luabind::scope Cls::RegisterLuaType(struct lua_State*, const char*);

MAKE_REGION(Region3)
MAKE_REGION(Region3f)
MAKE_REGION(Region2)
MAKE_REGION(Region1)

