#ifndef _RADIANT_CORE_DEFERRED_H
#define _RADIANT_CORE_DEFERRED_H

#include <vector>
#include <atomic>
#include <memory>
#include <sstream>
#include "ipromise.h"
#include "radiant_logger.h"
#include "radiant_macros.h"

#define DEFERRED_LOG(level)      LOG(deferred, level)

BEGIN_RADIANT_CORE_NAMESPACE

class DeferredBase {
public:
   DeferredBase() : id_(next_deferred_id_++) { };
   DeferredBase(std::string const& dbg_name) : dbg_name_(dbg_name), id_(next_deferred_id_++) {
      DEFERRED_LOG(5) << LogPrefix() << "creating new deferred";
   };
   ~DeferredBase() {
      DEFERRED_LOG(5) << LogPrefix() << "deferred destroyed";
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

   std::string GetDebugStr() const { return dbg_name_; }

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
      DEFERRED_LOG(5) << LogPrefix() << "creating new deferred";
   }

   ~Deferred() {
      if (IsPending()) {
         DEFERRED_LOG(5) << LogPrefix() << "rejecting deferred being destroying in wait state (this is probably an error!)";
         Reject(error_);
      }
   }

public: // the promise...
   void Done(CompleteFn cb) override {
      if (state_ == Resolved) {
         DEFERRED_LOG(5) << LogPrefix() << "done called while resolved. firing cb.";
         cb(result_);
      } else if (IsPending()) {
         DEFERRED_LOG(5) << LogPrefix() << "done called while waiting. buffering cb.";
         done_.push_back(cb);
      }
   }
   void Progress(typename CompleteFn cb) override {
      if (IsPending()) {         
         DEFERRED_LOG(5) << LogPrefix() << "progress called while waiting. buffering cb.";
         progress_.push_back(cb);
         if (state_ == InProgress) {
            DEFERRED_LOG(5) << LogPrefix() << "notifying progress of last notify state";
            cb(result_);
         }
      }      
   }
   void Fail(FailFn cb) override {
      if (state_ == Rejected) {
         DEFERRED_LOG(5) << LogPrefix() << "fail called while rejected. firing cb.";
         cb(error_);
      } else if (IsPending()) {
         DEFERRED_LOG(5) << LogPrefix() << "fail called while waiting. buffering cb.";
         fail_.push_back(cb);
      }
   }
   void Always(VoidFn cb) override {
      if (IsPending()) {
         DEFERRED_LOG(5) << LogPrefix() << "fail called while waiting. buffering cb.";
         always_.push_back(cb);
      } else {
         DEFERRED_LOG(5) << LogPrefix() << "fail called in non-wait state. firing cb.";
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
      try {
         if (IsPending()) {
            DEFERRED_LOG(5) << LogPrefix() << "resolve called in wait state. firing all done cbs.";
            state_ = Resolved;
            result_ = result;
            for (const auto &cb : done_) {
               cb(result_);
            }
            Cleanup();
         } else {
            DEFERRED_LOG(1) << LogPrefix() << "resolve called in non-wait state!";
         }
      } catch (std::exception &e) {
         DEFERRED_LOG(1) << LogPrefix() << " caught exception in Resolve: " << e.what();
      }
   }

   void Notify(CompleteT obj) {
      try {
         if (IsPending()) {
            DEFERRED_LOG(5) << LogPrefix() << "notify called in wait state. firing all progress cbs.";
            for (const auto &cb : progress_) {
               cb(obj);
            }
            result_ = obj;
            state_ = InProgress;
         } else {
            DEFERRED_LOG(1) << LogPrefix() << "notify called in non-wait state!";
         }
      } catch (std::exception &e) {
         DEFERRED_LOG(1) << LogPrefix() << " caught exception in Notify: " << e.what();
      }
   }
   void Reject(FailT const& reason) {
      try {
         if (IsPending()) {
            DEFERRED_LOG(5) << LogPrefix() << "reject called in wait state. firing all fail cbs.";
            state_ = Rejected;
            error_ = reason;
            for (const auto &fn : fail_) {
               fn(reason);
            }
            Cleanup();
         } else {
            DEFERRED_LOG(1) << LogPrefix() << "reject called in non-wait state!";
         }
      } catch (std::exception &e) {
         DEFERRED_LOG(1) << LogPrefix() << " caught exception in Reject: " << e.what();
      }
   }

private:
   void Cleanup() {
      try {
         if (state_ != InProgress) {
            for (const auto& fn : always_) {
               fn();
            }
            always_.clear();
            done_.clear();
            progress_.clear();
            fail_.clear();
         } else {
            DEFERRED_LOG(1) << LogPrefix() << "cleanup called in non-wait state!";
         }
      } catch (std::exception &e) {
         DEFERRED_LOG(1) << LogPrefix() << " caught exception in Reject: " << e.what();
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