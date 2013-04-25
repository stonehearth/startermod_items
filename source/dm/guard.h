#pragma once
#include "types.h"
#include "namespace.h"

BEGIN_RADIANT_DM_NAMESPACE

// xxx: this isn't a trace.  this is an object which calls another random
// function when it's destroyed.  traces fire callbacks when data changes.
// you would use one of these to automatically destroy a trace when some
// client object which has registered the callback is destroyed!
class Guard
{
public:
   Guard();
   Guard(std::function<void()> untrack);
   ~Guard();

   Guard(Guard&& other);
   const Guard& operator=(Guard&& rhs);

   void Reset();

private:
   NO_COPY_CONSTRUCTOR(Guard);

private:
   std::function<void()>   untrack_;
   int id_;
};

END_RADIANT_DM_NAMESPACE
