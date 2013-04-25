#ifndef _RADIANT_CSG_HEIGHTMAP_H
#define _RADIANT_CSG_HEIGHTMAP_H

#include "namespace.h"
#include "point.h"
#include <stdlib.h>
#include <vector>
#include <list>
#include "region.h"

struct lua_State;

BEGIN_RADIANT_CSG_NAMESPACE

template <class S>
class HeightMap
{
public:
   typedef S ScalarType;

public:
   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);

   HeightMap(int size, int scale) :
      size_(size),
      scale_(scale),
      samples_(size * size)
   {
   }

   template <class T> HeightMap(HeightMap<T> const& other) :
      size_(other.get_size()),
      scale_(other.get_scale())
   {
      samples_.reserve(size_ * size_);
      for (T const& sample : other) {
         samples_.emplace_back(static_cast<S>(sample));
      }
   }

   S get(int x, int y) const { return samples_[y * size_ + x]; }
   void set(int x, int y, S value) { samples_[y * size_ + x] = value; }
   int get_size() const { return size_; }
   int get_scale() const { return scale_; }

   std::vector<S> const& get_samples() const { return samples_; }
   typename std::vector<S>::const_iterator begin() const { return samples_.begin(); }
   typename std::vector<S>::const_iterator end() const { return samples_.end(); }

private:
   int               size_;
   int               scale_;
   std::vector<S>    samples_;
};

template <class S>
static std::ostream& operator<<(std::ostream& os, const HeightMap<S>& in)
{
   return (os << "(HeightMap size:" << in.get_size() << ")");
}

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_HEIGHTMAP_H
