#pragma once
#include <list>
#include <memory>

#include "radiant_macros.h"
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
   Guard(Guard&& other);
   ~Guard();

   int GetId() const { return id_; }

   const Guard& operator=(Guard&& rhs);
   const Guard& operator=(std::function<void()> untrack);

   const Guard& operator+=(Guard&& other);
   const Guard& operator+=(std::function<void()> untrack);

   void Clear();

private:
   NO_COPY_CONSTRUCTOR(Guard);
   void UntrackNodes();
   static int nextGuardId__;

private:
   int                                     id_;
   std::vector<std::function<void()>>      nodes_;
};

END_RADIANT_DM_NAMESPACE
