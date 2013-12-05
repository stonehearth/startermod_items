#include "radiant.h"
#include "radiant_stdutil.h"
#include "tracer_sync.h"

using namespace radiant;
using namespace radiant::dm;

TracerSync::TracerSync(std::string const& name) :
   Tracer(name)
{
}

TracerSync::~TracerSync()
{
}
