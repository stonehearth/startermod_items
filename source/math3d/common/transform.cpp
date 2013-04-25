#include "radiant.h"
#include "radiant.pb.h"
#include "transform.h"

using namespace radiant;
using namespace radiant::math3d;

math3d::transform math3d::transform::zero(point3(0.0f, 0.0f, 0.0f), quaternion(0.0f, 0.0f, 0.0f, 1.0f));


math3d::transform math3d::interpolate(const math3d::transform &t0, const math3d::transform &t1, float alpha)
{
   math3d::transform t;
   t.position = interpolate(t0.position, t1.position, alpha);
   t.orientation = interpolate(t0.orientation, t1.orientation, alpha);

   return t;
}

std::ostream& math3d::operator<<(std::ostream& out, const transform& source) {
   return out << "(pos: " << source.position << " ori: " << source.orientation << ")";
}
