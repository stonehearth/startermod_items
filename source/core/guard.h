#ifndef _RADIANT_CORE_GUARD_H
#define _RADIANT_CORE_GUARD_H

#include <vector>
#include <atomic>

#include "radiant_macros.h"
#include "namespace.h"

// TODO:
//   Allow these to tier. I.e. I want to add a guard which has 4 guards in
//   it to this guard, and when that gets destroyed, remove those 4 entries..
//   Use this guard everywhere we need guards...

BEGIN_RADIANT_CORE_NAMESPACE

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

   bool Empty() const;
   void Clear();

private:
   NO_COPY_CONSTRUCTOR(Guard);
   void UntrackNodes();
   static std::atomic<int> nextGuardId__;

private:
   int                                     id_;
   std::vector<std::function<void()>>      nodes_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_GUARD_H
