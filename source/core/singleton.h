#ifndef _RADIANT_CORE_SINGLETON_H
#define _RADIANT_CORE_SINGLETON_H

#include "namespace.h"
#include <memory>
#include <mutex>

BEGIN_RADIANT_CORE_NAMESPACE

template <typename Derived>
class Singleton
{
public:
   static Derived& GetInstance() {
      std::lock_guard<boost::detail::spinlock> lock(_lock);

      ASSERT(!destroyed_);
      if (destroyed_) {
         throw std::logic_error("attempting to access singleton after its destruction");
      }

      if (!constructed_) {
         ASSERT(!singleton_);
         constructed_ = true;
         singleton_.reset(new Derived());
      }
      return *singleton_;
   }

   static void Destroy() {
      std::lock_guard<boost::detail::spinlock> lock(_lock);

      ASSERT(!destroyed_);
      if (destroyed_) {
         throw std::logic_error("attempting 2nd destruction of singleton");
      }
      destroyed_ = true;
      singleton_ = nullptr;
   }

   virtual ~Singleton() {
      destroyed_ = true;
   }

protected:
   // Protect stack creation
   Singleton() { }

private:
   static std::unique_ptr<Derived> singleton_;
   static bool destroyed_;
   static bool constructed_;
   static boost::detail::spinlock _lock;
};

// Must be done once in a .cpp file somewhere to declare the singleton variable
#define DEFINE_SINGLETON(Cls) \
   std::unique_ptr<Cls> core::Singleton<Cls>::singleton_; \
   bool core::Singleton<Cls>::constructed_ = false; \
   bool core::Singleton<Cls>::destroyed_ = false; \
   boost::detail::spinlock core::Singleton<Cls>::_lock;

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_SINGLETON_H
