#include "pch.h"
#include "tile_address.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& om::operator<<(std::ostream& out, const TileAddress& addr)
{
   math3d::ipoint3 source(addr);
   out << std::fixed << '<' << source.x << ", " << source.y << ", " << source.z << '>';
   return out;
}
