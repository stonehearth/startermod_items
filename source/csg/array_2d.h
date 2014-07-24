#ifndef _RADIANT_CSG_ARRAY2D_H
#define _RADIANT_CSG_ARRAY2D_H

#include "namespace.h"
#include "point.h"
#include <stdlib.h>
#include <vector>
#include <list>
#include "region.h"

BEGIN_RADIANT_CSG_NAMESPACE

template <class S>
class Array2D
{
public:
   typedef S ScalarType;

public:
   Array2D(int width, int height) :
      width_(width),
      height_(height),
      origin_(csg::Point2::zero),
      data_(width_ * height_)
   {
   }

   Array2D(int width, int height, S initial) :
      width_(width),
      height_(height),
      origin_(csg::Point2::zero),
      data_(width_ * height_, initial)
   {
   }

   Array2D(int width, int height, csg::Point2 const& origin) :
      width_(width),
      height_(height),
      origin_(origin),
      data_(width_ * height_)
   {
   }

   Array2D(int width, int height, csg::Point2 const& origin, S initial) :
      width_(width),
      height_(height),
      origin_(origin),
      data_(width_ * height_, initial)
   {
   }

   S get(int x, int y) const {
      x -= origin_.x;
      y -= origin_.y;

      ASSERT(inbounds(x, y));
      if (inbounds(x, y)) {
         return data_[y * width_ + x];
      }
      return 0;
   }

   S fetch(int x, int y, S oob_value) const {
      x -= origin_.x;
      y -= origin_.y;

      if (inbounds(x, y)) {
         return data_[y * width_ + x];
      }
      return oob_value;
   }

   void set(int x, int y, S value) {
      x -= origin_.x;
      y -= origin_.y;

      ASSERT(inbounds(x, y));
      if (inbounds(x, y)) {
         data_[y * width_ + x] = value;
      }
   }

   int get_width() const { return width_; }
   int get_height() const { return height_; }

private:
   inline bool inbounds(int x, int y) const {
      return x >= 0 && y >=0 && x < width_ && y < height_;
   }

   typename std::vector<S>::const_iterator begin() const { return data_.begin(); }
   typename std::vector<S>::const_iterator end() const { return data_.end(); }

private:
   int               width_;
   int               height_;
   csg::Point2       origin_;
   std::vector<S>    data_;
};

template <class S>
static std::ostream& operator<<(std::ostream& os, const Array2D<S>& in)
{
   return (os << "(Array2D width:" << in.get_width() << " height:" << in.get_height() << ")");
}

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_ARRAY2D_H
