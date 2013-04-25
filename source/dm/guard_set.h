#pragma once
#include "guard.h"
#include <list>
#include <memory>

BEGIN_RADIANT_DM_NAMESPACE

// xxx: this isn't a trace.  this is an object which calls another random
// function when it's destroyed.  traces fire callbacks when data changes.
// you would use one of these to automatically destroy a trace when some
// client object which has registered the callback is destroyed!
class GuardSet
{
public:
   GuardSet();
   GuardSet(GuardSet&& other);
   ~GuardSet();

   const GuardSet& operator=(GuardSet&& rhs);

   void operator+=(Guard&& other);
   void Clear() { nodes_.clear(); }

private:
   NO_COPY_CONSTRUCTOR(GuardSet);

private:
   std::vector<Guard>      nodes_;
};

END_RADIANT_DM_NAMESPACE
