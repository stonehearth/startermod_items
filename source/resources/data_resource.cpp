#include "radiant.h"
#include "data_resource.h"

using namespace ::radiant;
using namespace ::radiant::resources;

std::ostream& ::radiant::resources::operator<<(std::ostream& out, const DataResource& source) {
   return (out << "[DataResource " << source.GetType() << "]");
}
