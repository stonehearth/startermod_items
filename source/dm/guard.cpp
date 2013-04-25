#include "pch.h"
#include "guard.h"
#include "store.h"

using namespace ::radiant;
using namespace ::radiant::dm;

static int id = 1;

Guard::Guard()
{
   id_ = id++;
   // std::cout << "Guard::Guard() " << id_ << std::endl;
}

Guard::Guard(std::function<void()> untrack) :
   untrack_(untrack)
{
   id_ = id++;
   // std::cout << "Guard::Guard(std::function<...>)  " << id_ << std::endl;
}

Guard::~Guard()
{
   // std::cout << "Guard::~Guard()  " << id_ << std::endl;
   if (untrack_) {
      // std::cout << "invoking untrack function!" << std::endl;
      untrack_();
   }
}

Guard::Guard(Guard&& other)
{
   id_ = id++;
   // std::cout << "Guard::Guard(Guard&& other)  " << id_ << " other is " << other.id_ << std::endl;

   *this = std::move(other);
}


const Guard& Guard::operator=(Guard&& other)
{
   // std::cout << "Guard::operator=(Guard&& other)  " << id_ << " other is " << other.id_ << std::endl;

   if (this != &other) {
      if (untrack_) {
         untrack_();
      }
      untrack_ = other.untrack_;
      other.untrack_ = nullptr;
   }
   return *this;
}

void Guard::Reset()
{
   if (untrack_) {
      untrack_();
      untrack_ = nullptr;
   }
}
