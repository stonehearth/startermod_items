#include "radiant.h"
#include "math3d.h"

using namespace radiant;
using namespace radiant::math3d;

ipoint3 ipoint3::origin(0, 0, 0);
ipoint3 ipoint3::one(1, 1, 1);

// xxx: this is confusing, right?  use the other one, exclusively
ipoint3::ipoint3(const radiant::math2d::ipoint2 &p, int extraDimension)
{
   switch (p.GetCoordinateSystem()) {
   case math2d::XZ_PLANE:
      x = (int)p.x;
      y = extraDimension;
      z = (int)p.y;
      break;
   case math2d::XY_PLANE:
      x = (int)p.x;
      y = (int)p.y;
      z = extraDimension;
      break;
   case math2d::YZ_PLANE:
      x = extraDimension;
      y = (int)p.y;
      z = (int)p.x;
      break;
   default:
      ASSERT(false);
   }
}

ipoint3::ipoint3(const radiant::math2d::ipoint2 &p, const math3d::ipoint3& offset)
{
   switch (p.GetCoordinateSystem()) {
   case math2d::XZ_PLANE:
      x = offset.x + p.x;
      y = offset.y;
      z = offset.z + p.y;
      break;
   case math2d::XY_PLANE:
      x = offset.x + p.x;
      y = offset.y + p.y;
      z = offset.z;
      break;
   case math2d::YZ_PLANE:
      x = offset.x;
      y = offset.y + p.y;
      z = offset.z + p.x;
      break;
   default:
      ASSERT(false);
   }
}

//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const ipoint3& source)
{
    out << std::fixed << '<' << source.x << ", " << source.y << ", " << source.z << '>';

    return out;
    
}   // End of operator<<()
