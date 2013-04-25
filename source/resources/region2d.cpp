#include "radiant.h"
#include "region2d.h"

using namespace ::radiant;
using namespace ::radiant::resources;

Resource::ResourceType Region2d::Type = REGION_2D;

std::ostream& ::radiant::resources::operator<<(std::ostream& out, const Region2d& source) {
   return (out << "[Region2d]");
}
