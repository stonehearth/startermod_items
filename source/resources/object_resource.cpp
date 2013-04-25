#include "radiant.h"
#include "object_resource.h"

using namespace ::radiant;
using namespace ::radiant::resources;

Resource::ResourceType ObjectResource::Type = OBJECT;

std::ostream& ::radiant::resources::operator<<(std::ostream& out, const ObjectResource& source) {
   return (out << "[ObjectResource]");
}
