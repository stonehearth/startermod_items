#include "pch.h"
#include "attributes.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

int Attributes::GetAttribute(std::string name) const
{
   const auto& attributes = attributes_.GetContents();
   auto i = attributes.find(name);
   return i != attributes.end() ? i->second : 0;
}

bool Attributes::HasAttribute(std::string name) const
{
   return attributes_.Contains(name);
}

void Attributes::SetAttribute(std::string name, int value)
{
   attributes_[name] = value;
}
