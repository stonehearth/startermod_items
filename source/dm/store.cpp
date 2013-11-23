#include "radiant.h"
#include "store.h"
#include "object.h"

using namespace ::radiant;
using namespace ::radiant::dm;

Store*  Store::stores_[4 + 1] = { 0 };

Store& Store::GetStore(int id)
{
   if (!id) {
      LOG(WARNING) << "object asked for store id 0.  we're about to die...";
   }
   ASSERT(id);
   return *stores_[id];
}

Store::Store(int which, std::string const& name) :
   storeId_(which),
   nextObjectId_(1),
   nextGenerationId_(1),
   name_(name)
{
   ASSERT(storeId_);

   // ASSERT(stores_[which] == nullptr);
   stores_[which] = this;
}

Store::~Store(void)
{
   Reset();
}

ObjectPtr Store::AllocObject(ObjectType t)
{
   ASSERT(allocators_[t]);

   ObjectPtr obj = allocators_[t]();
   obj->Initialize(*this, GetNextObjectId());
   OnAllocObject(obj);

   return obj;
}

ObjectPtr Store::AllocSlaveObject(ObjectType t, ObjectId id)
{
   ASSERT(nextObjectId_ == 1);
   ASSERT(allocators_[t]);

   ObjectPtr obj = allocators_[t]();
   obj->InitializeSlave(*this, id); 
   OnAllocObject(obj);

   return obj;
}

void Store::RegisterAllocator(ObjectType t, ObjectAllocFn allocator)
{
   ASSERT(!allocators_[t]);
   allocators_[t] = allocator;
}

void Store::Reset()
{
   dynamicObjects_.clear();

   ASSERT(objects_.size() == 0);
}

void Store::RegisterObject(Object& obj)
{
   ASSERT(obj.IsValid());
   if (objects_.find(obj.GetObjectId()) != objects_.end()) {
      throw std::logic_error(BUILD_STRING("data mode object " << obj.GetObjectId() << "being registered twice."));
   }

   objects_[obj.GetObjectId()] = &obj;

   // std::cout << "Store " << storeId_ << " RegisterObject(oid:" << obj.GetObjectId() << ") " << typeid(obj).name() << std::endl;
}


void Store::UnregisterObject(const Object& obj)
{
   // std::cout << "Store " << storeId_ << " UnregisterObject(oid:" << obj.GetObjectId() << ")" << std::endl;
   ValidateObjectId(obj.GetObjectId());

   ObjectId id = obj.GetObjectId();

   objects_.erase(id);
   dynamicObjects_.erase(id);
   auto i = traces_.find(id);
   if (i != traces_.end()) {
      stdutil::ForEachPrune<Trace>(i->second, [&](std::shared_ptr<Trace> t) {
         t->NotifyDestroyed();
      });
   }

   for (const auto& entry : tracers_) {
      entry.second->OnObjectDestroyed(id);
   }

   destroyed_.push_back(id);
}

void Store::ValidateObjectId(ObjectId oid) const
{
   ASSERT(oid == -1 || objects_.find(oid) != objects_.end());
}

int Store::GetObjectCount() const
{
   return objects_.size();
}

ObjectId Store::GetNextObjectId()
{
   return nextObjectId_++;
}

GenerationId Store::GetNextGenerationId()
{
   return nextGenerationId_++;
}

GenerationId Store::GetCurrentGenerationId()
{
   return nextGenerationId_;
}

void Store::OnAllocObject(std::shared_ptr<Object> obj)
{
   ASSERT(obj->IsValid());

   ObjectId id = obj->GetObjectId();
   dynamicObjects_[id] = DynamicObject(obj, obj->GetObjectType());

   for (auto const& entry : tracers_) {
      entry.second->OnObjectAlloced(obj);
   };
}

Object* Store::FetchStaticObject(ObjectId id) const
{
   auto i = objects_.find(id);
   return i == objects_.end() ? nullptr : i->second;
}


std::shared_ptr<Object> Store::FetchObject(int id, ObjectType type) const
{
   std::shared_ptr<Object> obj;

   auto i = dynamicObjects_.find(id);
   if (i != dynamicObjects_.end()) {
      const DynamicObject& entry = i->second;
      obj = entry.object.lock();
      if (obj) {
         ASSERT(obj->IsValid());
         // ASSERT(type == -1 || entry.type == type); Fails in cases where dynamic cast may succeed
      } else {
         dynamicObjects_.erase(i);
      }
   }
   return obj;
}  


std::vector<ObjectId> Store::GetModifiedSince(GenerationId when)
{
   std::vector<ObjectId> result;

   for (auto &entry : objects_) {
      Object* obj = entry.second;
      if (obj->GetLastModified() >= when) {
         ASSERT(obj->IsValid());
         result.push_back(obj->GetObjectId());
      }
   }
   return result;
}

void Store::OnObjectChanged(const Object& obj)
{
   ObjectId id = obj.GetObjectId();

   for (auto const& entry : tracers_) {
      entry.second->OnObjectChanged(id);
   }
}

bool Store::IsDynamicObject(ObjectId id)
{
   return dynamicObjects_.find(id) != dynamicObjects_.end();
}


AllocTracePtr Store::TraceAlloced(const char* reason, int category)
{
   return GetTracer(category)->TraceAlloced(*this, reason);
}

void Store::PushAllocState(AllocTrace& trace) const
{
   std::vector<ObjectPtr> objects;
   for (const auto& entry : dynamicObjects_) {
      ObjectPtr obj = entry.second.object.lock();
      if (obj) {
         objects.push_back(obj);
      }
   }
   trace.SignalUpdated(objects);
}
