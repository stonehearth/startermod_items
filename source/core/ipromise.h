#ifndef _RADIANT_CORE_PROMISE_H
#define _RADIANT_CORE_PROMISE_H

#include <functional>
#include "namespace.h"

BEGIN_RADIANT_CORE_NAMESPACE

template <typename CompleteT, typename FailT>
class IPromise
{
public:
   typedef std::function<void(CompleteT const&)> CompleteFn;
   typedef std::function<void(FailT const&)> FailFn;
   typedef std::function<void()> VoidFn;

public:
   virtual void Done(CompleteFn cb) = 0;
   virtual void Progress(typename CompleteFn cb) = 0;
   virtual void Fail(FailFn cb) = 0;
   virtual void Always(VoidFn cb) = 0;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_PROMISE_H