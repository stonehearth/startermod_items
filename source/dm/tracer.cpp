#include "radiant.h"
#include "tracer.h"

using namespace radiant;
using namespace radiant::dm;

Tracer::Tracer(std::string const& name) :
   name_(name)
{
}

Tracer::~Tracer()
{
}
