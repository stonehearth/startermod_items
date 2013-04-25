#ifndef _RADIANT_CLIENT_WAIT_FOR_ENTITY_ALLOC_H
#define _RADIANT_CLIENT_WAIT_FOR_ENTITY_ALLOC_H

#include "pending_command.h"
#include "om/om.h"
#include "dm/dm.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class WaitForEntityAlloc : public std::enable_shared_from_this<WaitForEntityAlloc>
{
public:
   WaitForEntityAlloc() : id_(0) { }

   WaitForEntityAlloc(dm::Store& store, dm::ObjectId id, std::function<void()> fn) :
      id_(0)
   {
      // this object is stack based and will go away.  create a new object which
      // will hold a reference to itself until the requested object is created
      if (id) {
         if (store.FetchObject(id, -1)) {
            fn();
         } else {
            LOG(WARNING) << "Waiting for object alloc: " << id;
            std::make_shared<WaitForEntityAlloc>()->Install(store, id, fn);
         }
      }
   }

   ~WaitForEntityAlloc() {
      if (id_) {
         LOG(WARNING) << "Object " << id_ << " alloced.";
      }
   }

private:
   void Install(dm::Store& store, dm::ObjectId id, std::function<void()> fn) {
      auto self = shared_from_this();
      id_ = id;
      guard_ = store.TraceDynamicObjectAlloc([self, id, fn](const dm::ObjectPtr obj) {
         dm::Guard destructor;
         if (obj->GetObjectId() == id) {
            fn();
            destructor = std::move(self->guard_);
         }
      });
   }

private:
   dm::ObjectId   id_;
   dm::Guard      guard_;
};


END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_WAIT_FOR_ENTITY_ALLOC_H
