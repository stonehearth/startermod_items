#include "pch.h"
#include "guard.h"
#include <utility>

using namespace ::radiant;
using namespace ::radiant::dm;

Guard::Guard()
{
}

Guard::Guard(Guard&& other) :
   nodes_(std::move(other.nodes_))
{
}

Guard::Guard(std::function<void()> untrack)
{
   (*this) += untrack;
}

Guard::~Guard()
{
   UntrackNodes();
}

Guard const& Guard::operator=(Guard&& other)
{
   if (this != &other) {
      UntrackNodes();
      nodes_ = std::move(other.nodes_);
   }
   return *this;
}

Guard const& Guard::operator+=(Guard&& other)
{   
   nodes_.insert(nodes_.end(), other.nodes_.begin(), other.nodes_.end());
   other.nodes_.clear();
   return *this;
}

Guard const& Guard::operator=(std::function<void()> untrack)
{
   UntrackNodes();
   nodes_.push_back(untrack);
   return *this;
}

Guard const& Guard::operator+=(std::function<void()> untrack)
{
   nodes_.push_back(untrack);
   return *this;
}

void Guard::Clear()
{
   UntrackNodes();
}

void Guard::UntrackNodes()
{
   for (auto &fn : nodes_) {
      fn();
   }
   nodes_.clear();
}

