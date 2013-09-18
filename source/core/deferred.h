#ifndef _RADIANT_CORE_DEFERRED_H
#define _RADIANT_CORE_DEFERRED_H

#include <vector>
#include <atomic>
#include <memory>
#include <sstream>
#include "ipromise.h"
#include "radiant_logger.h"
#include "radiant_macros.h"

BEGIN_RADIANT_CORE_NAMESPACE

class DeferredBase {
public:
   DeferredBase() : id_(next_deferred_id_++) { };
   DeferredBase(std::string const& dbg_name) : dbg_name_(dbg_name), id_(next_deferred_id_++) {
      LOG(INFO) << LogPrefix() << "creating new deferred";
   };
   ~DeferredBase() {
      LOG(INFO) << LogPrefix() << "deferred destroyed";
   }

   std::string LogPrefix() const {
      std::ostringstream os;
      os << "[deferred " << id_ << " ";
      if (!dbg_name_.empty()) {
         os << "'" << dbg_name_ << "'";
      }
      os << "] ";
      return os.str();
   }

protected:
   typedef int DeferredId;
   static std::atomic<DeferredId> next_deferred_id_;

protected:
   std::string             dbg_name_;
   DeferredId              id_;
};

template <typename CompleteT, typename FailT>
class Deferred :
   public DeferredBase,
   public IPromise<CompleteT, FailT>
{
public:
   Deferred(std::string const& dbg_name) : DeferredBase(dbg_name), state_(Initialized) { }

   Deferred() : DeferredBase(), state_(Initialized) {
      LOG(INFO) << LogPrefix() << "creating new deferred";
   }

   ~Deferred() {
      if (IsPending()) {
         LOG(INFO) << LogPrefix() << "rejecting deferred being destroying in wait state (this is probably an error!)";
         Reject(error_);
      }
   }

public: // the promise...
   void Done(CompleteFn cb) override {
      if (state_ == Resolved) {
         LOG(INFO) << LogPrefix() << "done called while resolved. firing cb.";
         cb(result_);
      } else if (IsPending()) {
         LOG(INFO) << LogPrefix() << "done called while waiting. buffering cb.";
         done_.push_back(cb);
      }
   }
   void Progress(typename CompleteFn cb) override {
      if (IsPending()) {         
         LOG(INFO) << LogPrefix() << "progress called while waiting. buffering cb.";
         progress_.push_back(cb);
         if (state_ == InProgress) {
            LOG(INFO) << LogPrefix() << "notifying progress of last notify state";
            cb(result_);
         }
      }      
   }
   void Fail(FailFn cb) override {
      if (state_ == Rejected) {
         LOG(INFO) << LogPrefix() << "fail called while rejected. firing cb.";
         cb(error_);
      } else if (IsPending()) {
         LOG(INFO) << LogPrefix() << "fail called while waiting. buffering cb.";
         fail_.push_back(cb);
      }
   }
   void Always(VoidFn cb) override {
      if (IsPending()) {
         LOG(INFO) << LogPrefix() << "fail called while waiting. buffering cb.";
         always_.push_back(cb);
      } else {
         LOG(INFO) << LogPrefix() << "fail called in non-wait state. firing cb.";
         cb();
      }
   }

public:  // the deferred...
   int GetId() const {
      return id_;
   }
   std::string GetStateName() const {
      static const char* state_names[] = {
         "initialized", "inprogress", "resolved", "rejected"
      };
      return state_names[state_];
   }

   bool IsPending() const {
      return state_ == InProgress || state_ == Initialized;
   }

   void Resolve(CompleteT result) {
      if (IsPending()) {
         LOG(INFO) << LogPrefix() << "resolve called in wait state. firing all done cbs.";
         state_ = Resolved;
         result_ = result;
         for (const auto &cb : done_) {
            cb(result_);
         }
         Cleanup();
      } else {
         LOG(ERROR) << LogPrefix() << "resolve called in non-wait state!";
      }
   }

   void Notify(CompleteT obj) {
      if (IsPending()) {
         LOG(INFO) << LogPrefix() << "notify called in wait state. firing all progress cbs.";
         for (const auto &cb : progress_) {
            cb(obj);
         }
         result_ = obj;
         state_ = InProgress;
      } else {
         LOG(ERROR) << LogPrefix() << "notify called in non-wait state!";
      }
   }
   void Reject(FailT const& reason) {
      if (IsPending()) {
         LOG(INFO) << LogPrefix() << "reject called in wait state. firing all fail cbs.";
         state_ = Rejected;
         error_ = reason;
         for (const auto &fn : fail_) {
            fn(reason);
         }
         Cleanup();
      } else {
         LOG(ERROR) << LogPrefix() << "reject called in non-wait state!";
      }
   }

private:
   void Cleanup() {
      if (state_ != InProgress) {
         for (const auto& fn : always_) {
            fn();
         }
         always_.clear();
         done_.clear();
         progress_.clear();
         fail_.clear();
      } else {
         LOG(ERROR) << LogPrefix() << "cleanup called in non-wait state!";
      }
   }

private:
   NO_COPY_CONSTRUCTOR(Deferred)

   enum State {
      Initialized = 0,
      InProgress = 1,
      Resolved = 2,
      Rejected = 3,
   };

private:
   std::vector<VoidFn>     always_;
   std::vector<CompleteFn> done_;
   std::vector<CompleteFn> progress_;
   std::vector<FailFn>     fail_;

   CompleteT               result_;
   FailT                   error_;
   State                   state_;
};

template <typename CompleteT, typename FailT>
std::ostream& operator<<(std::ostream& os, std::shared_ptr<Deferred<CompleteT, FailT>> p)
{
   if (p) {
      return (os << "[deferred null]");
   }
   return (os << *p);
}

template <typename CompleteT, typename FailT>
std::ostream& operator<<(std::ostream& os, Deferred<CompleteT, FailT> const& f) {
   return (os << "[" << f.LogPrefix() << " state: " << f.GetStateName() << "]");
}

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_DEFERRED_H