#include "radiant.h"
#include "common.h"
#include "ipoint2.h"
#include "ipoint3.h"
#include "point3.h"
#include "vector3.h"
#include "quaternion.h"

using namespace radiant;
using namespace radiant::math2d;

//===============================================================================
// @ ipoint2.cpp
// 
// 3D vector class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

ipoint2::ipoint2(int _x, int _y, CoordinateSystem cs) :
   x(_x), y(_y), space_(cs)
{
}

static struct {
   int x, y;
} coordmap3 [] = {
   { 0, 2 },  // XZ_PLANE (x -> x, z -> y)
   { 0, 1 },  // XY_PLANE (x -> x, y -> y)
   { 2, 1 },  // YZ_PLANE (z -> x, y -> y)
};

ipoint2::ipoint2(const radiant::math3d::point3 &p, CoordinateSystem cs) :
   space_(cs)
{
   ASSERT(cs == XZ_PLANE || cs == XY_PLANE || cs == YZ_PLANE);

   x = (int)p[coordmap3[cs].x];
   y = (int)p[coordmap3[cs].y];
}

ipoint2::ipoint2(const radiant::math3d::ipoint3 &p, CoordinateSystem cs) :
   space_(cs)
{
   ASSERT(cs == XZ_PLANE || cs == XY_PLANE || cs == YZ_PLANE);

   x = p[coordmap3[cs].x];
   y = p[coordmap3[cs].y];
}

math3d::quaternion ipoint2::GetQuaternion() const
{
   math3d::vector3 origin(0, 0, 1);
   math3d::vector3 vector(math3d::point3(math3d::ipoint3(*this, 0)));
   return math3d::quaternion(origin, vector);
}

ipoint2 ipoint2::ProjectOnto(CoordinateSystem cs, int width) const
{
   if (cs == space_) {
      return *this;
   }
   /* weeriiirdd.... */
   switch (space_) {
   case XZ_PLANE:
      /* In the XZ plane, the x === x coordinate, y === z coordinate.
       *   into XY plane, we need to preserve x (stored in x component), which means:
       *        dst(x) <- src(x) and dst(y) <- width
       *   into YZ plane, we need to preserve Z (stored in x component), which means:
       *        dst(x) <- src(y) and dst(y) <- width
       */
      return cs == XY_PLANE ? ipoint2(x, width, cs) : ipoint2(y, width, cs) /* YZ_PLANE */;

   case XY_PLANE:
      /* In the XY plane, the x === x coordinate, y === y coordinate.
       *   into XZ plane, we need to preserve x (stored in x component), which means:
       *        dst(x) <- src(x) and dst(y) <- width
       *   into YZ plane, we need to preserve Y (stored in y component), which means:
       *        dst(x) <- width and dst(y) <- src(y)
       */
      return cs == XZ_PLANE ? ipoint2(x, width, cs) : ipoint2(width, y, cs) /* YZ_PLANE */;

   case YZ_PLANE:
      /* In the YZ plane, the x === z coordinate, y === y coordinate.
       *   into XZ plane, we need to preserve z (stored in y component), which means:
       *        dst(x) <- width and dst(y) <- x
       *   into XY plane, we need to preserve Y (stored in y component), which means:
       *        dst(x) <- width and dst(y) <- y
       */
      return cs == XZ_PLANE ? ipoint2(width, x, cs) : ipoint2(width, y, cs) /* XY_PLANE */;
   }
   ASSERT(false);
   return ipoint2(width, width, cs);
}

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math2d::operator<<(std::ostream& out, const ipoint2& source)
{
    out << '<' << source.x << ',' << source.y << ',' << '>';

    return out;
    
}   // End of operator<<()
    
//-------------------------------------------------------------------------------
// @ ipoint2::length_squared()
//-------------------------------------------------------------------------------
// Vector length squared (avoids square root)
//-------------------------------------------------------------------------------
int 
ipoint2::length_squared() const
{
    return (x*x + y*y);

}   // End of ipoint2::length_squared()

//-------------------------------------------------------------------------------
// @ ipoint2::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
ipoint2::operator==(const ipoint2& other) const
{
   ASSERT(space_ == other.space_);
   return x == other.x && y == other.y;
}   // End of ipoint2::operator==()


//-------------------------------------------------------------------------------
// @ ipoint2::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
ipoint2::operator!=(const ipoint2& other) const
{
   ASSERT(space_ == other.space_);
   return x != other.x || y != other.y;
}   // End of ipoint2::operator!=()


//-------------------------------------------------------------------------------
// @ vector2math::is_zero()
//-------------------------------------------------------------------------------
// Check for zero vector
//-------------------------------------------------------------------------------
bool 
ipoint2::is_zero() const
{
   return x == 0 && y == 0;
}   // End of vector2math::is_zero()


//-------------------------------------------------------------------------------
// @ ipoint2::clean()
//-------------------------------------------------------------------------------
// Set elements close to zero equal to zero
//-------------------------------------------------------------------------------
void
ipoint2::clean()
{
   x = 0;
   y = 0;
}   // End of ipoint2::clean()

//-------------------------------------------------------------------------------
// @ ipoint2::operator+()
//-------------------------------------------------------------------------------
// Add vector to self and return
//-------------------------------------------------------------------------------
ipoint2 
ipoint2::operator+(const ipoint2& other) const
{
   ASSERT(space_ == other.space_);
   return ipoint2(x + other.x, y + other.y, space_);
}   // End of ipoint2::operator+()



//-------------------------------------------------------------------------------
// @ ipoint2::operator-()
//-------------------------------------------------------------------------------
// Subtract vector from self and return
//-------------------------------------------------------------------------------
ipoint2 
ipoint2::operator-(const ipoint2& other) const
{
   ASSERT(space_ == other.space_);
   return ipoint2(x - other.x, y - other.y, space_);
}   // End of ipoint2::operator-()


//-------------------------------------------------------------------------------
// @ ipoint2::operator-=()
//-------------------------------------------------------------------------------
// Subtract vector from self, store in self
//-------------------------------------------------------------------------------
ipoint2& 
operator-=(ipoint2& self, const ipoint2& other)
{
    ASSERT(self.space_ == other.space_);

    self.x -= other.x;
    self.y -= other.y;

    return self;
}   // End of ipoint2::operator-=()

//-------------------------------------------------------------------------------
// @ ipoint2::operator-() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
ipoint2
ipoint2::operator-() const
{
    return ipoint2(-x, -y, space_);
}    // End of ipoint2::operator-()


//-------------------------------------------------------------------------------
// @ operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication
//-------------------------------------------------------------------------------
ipoint2   
ipoint2::operator*(int scalar)
{
    return ipoint2(scalar*x, scalar*y, space_);

}   // End of operator*()


//-------------------------------------------------------------------------------
// @ operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication
//-------------------------------------------------------------------------------
ipoint2   
operator*(int scalar, const ipoint2& vector)
{
   return ipoint2(scalar*vector.x, scalar*vector.y, vector.GetCoordinateSystem());

}   // End of operator*()


//-------------------------------------------------------------------------------
// @ ipoint2::operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication by self
//-------------------------------------------------------------------------------
ipoint2&
ipoint2::operator*=(int scalar)
{
    x *= scalar;
    y *= scalar;

    return *this;

}   // End of ipoint2::operator*=()


//-------------------------------------------------------------------------------
// @ operator/()
//-------------------------------------------------------------------------------
// Scalar division
//-------------------------------------------------------------------------------
ipoint2   
ipoint2::operator/(int scalar)
{
    return ipoint2(x/scalar, y/scalar, space_);

}   // End of operator/()


//-------------------------------------------------------------------------------
// @ ipoint2::operator/=()
//-------------------------------------------------------------------------------
// Scalar division by self
//-------------------------------------------------------------------------------
ipoint2&
ipoint2::operator/=(int scalar)
{
    x /= scalar;
    y /= scalar;

    return *this;

}   // End of ipoint2::operator/=()


//-------------------------------------------------------------------------------
// @ ipoint2::dot()
//-------------------------------------------------------------------------------
// dot product by self
//-------------------------------------------------------------------------------
int               
ipoint2::dot(const ipoint2& vector) const
{
   ASSERT(space_ == vector.space_);
   return (x*vector.x + y*vector.y);

}   // End of ipoint2::dot()


//-------------------------------------------------------------------------------
// @ dot()
//-------------------------------------------------------------------------------
// dot product friend operator
//-------------------------------------------------------------------------------
int               
dot(const ipoint2& vector1, const ipoint2& pt)
{
   ASSERT(vector1.space_ == pt.space_);
   return (vector1.x*pt.x + vector1.y*pt.y); 
}   // End of dot()


//-------------------------------------------------------------------------------
// @ ipoint2::perp_dot()
//-------------------------------------------------------------------------------
// Perpendicular dot product by self
//-------------------------------------------------------------------------------
int               
ipoint2::perp_dot(const ipoint2& vector) const
{
   ASSERT(space_ == vector.space_);
   return (x*vector.y - y*vector.x);
}   // End of ipoint2::dot()


//-------------------------------------------------------------------------------
// @ dot()
//-------------------------------------------------------------------------------
// dot product friend operator
//-------------------------------------------------------------------------------
int               
perp_dot(const ipoint2& vector1, const ipoint2& pt)
{
   ASSERT(vector1.space_ == pt.space_);
   return (vector1.x*pt.y - vector1.y*pt.x);
}   // End of dot()

