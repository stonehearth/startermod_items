#include "pch.h"
#include "guard_set.h"
#include <utility>

using namespace ::radiant;
using namespace ::radiant::dm;

GuardSet::GuardSet()
{
}

GuardSet::GuardSet(GuardSet&& other) :
   nodes_(std::move(other.nodes_))
{
}


GuardSet::~GuardSet()
{
}

void GuardSet::operator+=(Guard&& other)
{   
   nodes_.push_back(std::forward<Guard>(other));
}

const GuardSet& GuardSet::operator=(GuardSet&& other)
{
   if (this != &other) {
      nodes_ = std::move(other.nodes_);
   }
   return *this;
}

