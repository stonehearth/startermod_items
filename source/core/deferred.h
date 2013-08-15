#ifndef _RADIANT_CORE_DEFERRED_H
#define _RADIANT_CORE_DEFERRED_H

#include <libjson.h>
#include "namespace.h"

BEGIN_RADIANT_CORE_NAMESPACE

template <class Derived, typename DerivedCompleteFn>
class Deferred
{
public:
   Deferred() : state_(Waiting) {}
   ~Deferred() {
      if (state_ == Waiting) {
         JSONNode error;
         error.push_back(JSONNode("error", "deferred destroyed without reporting success or failure"));
         Reject(error);
      }
   }

   typedef std::function<void()> VoidFn;
   typedef std::function<void(JSONNode const&)> FailFn;
   typedef std::function<void(typename DerivedCompleteFn)> CompleteDelegateFn;

public:   // the promise...
   void Done(typename DerivedCompleteFn cb) {
      if (state_ == Resolved) {
         doneDelegate_(cb);
      } else if (state_ == Waiting) {
         done_.push_back(cb);
      }
   }
   void Progress(typename DerivedCompleteFn cb) {
      if (state_ == Waiting) {
         progress_.push_back(cb);
      }
   }
   void Fail(FailFn cb) {
      if (state_ == Rejected) {
         cb(error_);
      } else if (state_ == Waiting) {
         fail_.push_back(cb);
      }
   }
   void Always(VoidFn cb) {
      if (state_ == Waiting) {
         always_.push_back(cb);
      } else {
         cb();
      }
   }

public:  // the deferred...
   void ResolveImpl(CompleteDelegateFn delegate_fn) {
      ASSERT(state_ == Waiting);
      state_ = Resolved;
      doneDelegate_ = delegate_fn;
      for (const auto &fn : done_) {
         doneDelegate_(fn);
      }
      Cleanup();
   }
   void NotifyImpl(CompleteDelegateFn delegate_fn) {
      ASSERT(state_ == Waiting);
      for (const auto &fn : progress_) {
         delegate_fn(fn);
      }
   }
   void Reject(JSONNode const& n) {
      ASSERT(state_ == Waiting);
      state_ = Rejected;
      error_ = n;
      for (const auto &fn : fail_) {
         fn(n);
      }
      Cleanup();
   }
   void Cleanup() {
      ASSERT(state_ != Waiting);
      for (const auto& fn : always_) {
         fn();
      }
      always_.clear();
      done_.clear();
      progress_.clear();
      fail_.clear();
   }

private:
   enum State {
      Waiting = 0,
      Resolved = 1,
      Rejected = 2,
   };

private:
   std::vector<VoidFn>     always_;
   std::vector<typename DerivedCompleteFn> done_;
   std::vector<typename DerivedCompleteFn> progress_;
   CompleteDelegateFn      doneDelegate_;
   std::vector<FailFn>     fail_;
   JSONNode                error_;
   State                   state_;
};

template <typename T0>
class Deferred1 : public Deferred<Deferred1<T0>, std::function<void(T0 const&)>>
{
public:
   typedef T0 Type0;
   typedef std::function<void(T0 const&)> CompleteFn;

   void Resolve(T0 const& r) {
      result_ = r;
      ResolveImpl([=](CompleteFn fn) { fn(result_); });
   }

   void Notify(T0 const& r) {
      NotifyImpl([=](CompleteFn fn) { fn(r); });
   }

protected:
   T0                      result_;
};

template <typename T0, typename T1>
class Deferred2 : public Deferred<Deferred2<T0, T1>, std::function<void(T0 const&, T1 const &)>>
{
public:
   typedef T0 Type0;
   typedef T1 Type1;
   typedef std::function<void(T0 const&, T1 const &)> CompleteFn;

   void Resolve(T0 const& r, T1 const& s) {
      r_ = r;
      s_ = s;
      ResolveImpl([=](CompleteFn fn) { fn(r_, s_); });
   }

   void Notify(T0 const& r, T1 const& s) {
      NotifyImpl([=](CompleteFn fn) { fn(r, s); });
   }

protected:
   T0    r_;
   T1    s_;
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_DEFERRED_H