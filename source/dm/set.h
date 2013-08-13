#pragma once
#include "object.h"
#include "store.pb.h"
#include <vector>
#include "namespace.h"
#include "radiant_luabind.h"

BEGIN_RADIANT_DM_NAMESPACE

template <class T>
class Set : public Object
{
public:
   typedef T ValueType;
   typedef std::vector<T> ContainerType;

   //static decltype(Protocol::Set::contents) extension;
   DEFINE_DM_OBJECT_TYPE(Set);
   Set() : Object() { }

   const std::vector<T>& GetContents() const { return items_; }

   typename ContainerType::const_iterator RemoveIterator(typename ContainerType::const_iterator i) {
      const T& item = *i;
      stdutil::UniqueRemove(added_, item);
      removed_.push_back(item);
      MarkChanged();
      return items_.erase(i); // this is O(n) !!
   }

   void Clear() {
      while (!items_.empty()) {
         Remove(items_.back());
      }
   }
   void Remove(const T& item) {
      // this could be faster...
      if (stdutil::UniqueRemove(items_, item)) {
         if (!stdutil::UniqueRemove(added_, item)) {
            removed_.push_back(item);
         }
         MarkChanged();
      }
   }

   void Insert(const T& item) {
      if (!stdutil::contains(items_, item)) {
         items_.push_back(item);
         added_.push_back(item);
         stdutil::UniqueRemove(removed_, item);
         MarkChanged();
      }
   }

   bool IsEmpty() const {
      return items_.empty();
   }

   int Size() const {
      return items_.size();
   }

   bool Contains(const T& item) {
      return stdutil::contains(items_, item);
   }

   dm::Guard TraceSetChanges(const char* reason,
                             std::function<void(const T& v)> added,
                             std::function<void(const T& v)> removed) const
   {
      auto cb = [=]() {
         if (removed) {
            for (const auto& key : removed_) {
               removed(key);
            }
         }
         if (added) {
            for (const auto& key : added_) {
               added(key);
            }
         }
      };
      return TraceObjectChanges(reason, cb);
   }

   class LuaPromise
   {
   public:
      LuaPromise(const Set& c) {
         auto changed = std::bind(&LuaPromise::OnChange, this, std::placeholders::_1);
         auto removed = std::bind(&LuaPromise::OnRemove, this, std::placeholders::_1);

         guard_ = c.TraceSetChanges("set promise", changed, removed);
      }

      ~LuaPromise() {
         LOG(WARNING) << "killing Promise...";
      }

   public:
      void Destroy() {
         guard_.Clear();
      }

      LuaPromise* PushAddedCb(luabind::object cb) {
         changedCbs_.push_back(cb);
         return this;
      }

      LuaPromise* PushRemovedCb(luabind::object cb) {
         removedCbs_.push_back(cb);
         return this;
      }

      void OnChange(const T& value) {
         for (auto& cb : changedCbs_) {
            luabind::call_function<void>(cb, value);
         }
      }
      void OnRemove(const T& value)  {
         for (auto& cb : removedCbs_) {
            luabind::call_function<void>(cb, value);
         }
      }

   private:
      dm::Guard                     guard_;
      std::vector<luabind::object>  changedCbs_;
      std::vector<luabind::object>  removedCbs_;
   };

   LuaPromise* CreateLuaPromise() const {
      return new LuaPromise(*this);
   }

   typename std::vector<T>::const_iterator begin() const { return items_.begin(); }
   typename std::vector<T>::const_iterator end() const { return items_.end(); }

   template <class T>
   class LuaIterator {
   public:
      LuaIterator(const T& container) : container_(container) {
         i_ = container_.begin();
      }

      void NextIteration(lua_State *L, luabind::object s, luabind::object var) {
         if (i_ != container_.end()) {
            luabind::object(L, *i_).push(L);
            i_++;
         } else {
            lua_pushnil(L);
         }
      }

   private:
      const T& container_;
      typename T::const_iterator i_;
   };

   static void LuaIteratorStart(lua_State *L, const Set<T>& s)
   {
      using namespace luabind;
      object(L, new LuaIterator<decltype(items_)>(s.items_)).push(L); // f
      object(L, 1).push(L); // s (ignored)
      object(L, 1).push(L); // var (ignored)
   }

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name) {
      using namespace luabind;
      std::string itername = std::string(name) + "Iterator";
      std::string promisename = std::string(name) + "Promise";
      return
         class_<Set<T>>(name)
            .def(tostring(const_self))
            .def("size",              &Set<T>::Size)
            .def("is_empty",          &Set<T>::IsEmpty)
            .def("items",             &Set<T>::LuaIteratorStart)
            .def("trace",             &Set<T>::CreateLuaPromise)
         ,
         class_<LuaIterator<decltype(items_)>>(itername.c_str())
            .def("__call",    &LuaIterator<decltype(items_)>::NextIteration)
         ,
         class_<LuaPromise>(itername.c_str())
            .def("on_added",    &LuaPromise::PushAddedCb)
            .def("on_removed",  &LuaPromise::PushRemovedCb)
            .def("destroy",  &LuaPromise::Destroy)
         ;
   }

public:
   void SaveValue(const Store& store, Protocol::Value* valmsg) const {
      Protocol::Set::Update* msg = valmsg->MutableExtension(Protocol::Set::extension);
      for (const T& item : added_) {
         Protocol::Value* submsg = msg->add_added();
         SaveImpl<T>::SaveValue(store, submsg, item);
      }
      for (const T& item : removed_) {
         Protocol::Value* submsg = msg->add_removed();
         SaveImpl<T>::SaveValue(store, submsg, item);
      }
      removed_.clear();
      added_.clear();
   }
   void LoadValue(const Store& store, const Protocol::Value& valmsg) {
      const Protocol::Set::Update& msg = valmsg.GetExtension(Protocol::Set::extension);
      T value;

      removed_.clear();
      added_.clear();

      for (const Protocol::Value& item : msg.added()) {
         SaveImpl<T>::LoadValue(store, item, value);
         added_.push_back(value);
         items_.push_back(value);
      }
      for (const Protocol::Value& item : msg.removed()) {
         SaveImpl<T>::LoadValue(store, item, value);
         removed_.push_back(value);
         stdutil::UniqueRemove(items_, value);
      }
   }
   
private:
   NO_COPY_CONSTRUCTOR(Set<T>);

private:
   std::vector<T>          items_;
   mutable std::vector<T>  added_;
   mutable std::vector<T>  removed_;
};

template <typename T>
static std::ostream& operator<<(std::ostream& os, const Set<T>& in)
{
   return in.ToString(os);
}

END_RADIANT_DM_NAMESPACE
