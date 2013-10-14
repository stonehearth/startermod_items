#include "radiant.h"
#include "guard.h"
#include <utility>

using namespace ::radiant;
using namespace ::radiant::core;

std::atomic<int> Guard::nextGuardId__(1);

Guard::Guard() :
   id_(nextGuardId__++)
{
}

Guard::Guard(Guard&& other) :
   id_(nextGuardId__++),
   nodes_(std::move(other.nodes_))
{
}

Guard::Guard(std::function<void()> untrack) :
   id_(nextGuardId__++)
{
   (*this) += untrack;
}

Guard::~Guard()
{
   //LOG(WARNING) << "destroying guard " << id_;
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

bool Guard::Empty() const
{
   return nodes_.empty();
}


