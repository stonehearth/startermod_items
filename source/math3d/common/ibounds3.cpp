#include "radiant.h"
#include "math3d.h"

using namespace radiant;
using namespace radiant::math3d;

ibounds3::ibounds3(aabb& a)
{
   *this = a.ToBounds();
}

std::ostream& math3d::operator<<(std::ostream& out, const ibounds3& source)
{
   out << "min [" << source._min << "] max[" << source._max << "]";
   return out;
}

void ibounds3::Clean()
{
   for (int i = 0; i < 3; i++) {
      if (_min[i] > _max[i]) {
         std::swap(_min[i], _max[i]);
      }
   }
}
