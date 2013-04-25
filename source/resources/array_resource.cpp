#include "radiant.h"
#include "array_resource.h"

using namespace ::radiant;
using namespace ::radiant::resources;

Resource::ResourceType ArrayResource::Type = ARRAY;

std::ostream& ::radiant::resources::operator<<(std::ostream& out, const ArrayResource& source) {
   return (out << "[ArrayResource]");
}
