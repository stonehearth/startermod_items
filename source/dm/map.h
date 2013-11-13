#pragma once
#include "object.h"
#include "store.pb.h"
#include <unordered_map>
#include "namespace.h"
#include "radiant.h"
#include "radiant_stdutil.h"
#include "lib/lua/bind.h"
#include "lib/lua/register.h"
#include "dbg_indenter.h"

BEGIN_RADIANT_DM_NAMESPACE

// TODO: make sure only non-Object types get used in containers...
template <class K, class V, class Hash=std::unordered_map<K, V>::hasher>
class Map : public Object
{
public:
   DEFINE_DM_OBJECT_TYPE(Map, map);

   typedef K KeyType;
   typedef V ValueType;

   Map() : Object() { }

   void GetDbgInfo(DbgInfo &info) const override {
      if (WriteDbgInfoHeader(info)) {
         info.os << " {" << std::endl;
         {
            Indenter indent(info.os);
            auto i = items_.begin(), end = items_.end();
            while (i != end) {
               SaveImpl<K>::GetDbgInfo(i->first, info);
               info.os << " : ";
               SaveImpl<V>::GetDbgInfo(i->second, info);
               if (++i != end) {
                  info.os << ",";
               }
               info.os << std::endl;
            }
         }
         info.os << "}";
      }
   }

   typedef std::unordered_map<K, V, Hash> ContainerType;

   int GetSize() const { return items_.size(); }
   bool IsEmpty() const { return items_.empty(); }

   const ContainerType& GetContents() const { return items_; }

   typename ContainerType::const_iterator begin() const { return items_.begin(); }
   typename ContainerType::const_iterator end() const { return items_.end(); }
   typename ContainerType::const_iterator find(const K& key) const { return items_.find(key); }

   typename ContainerType::const_iterator RemoveIterator(typename ContainerType::const_iterator i) {
      const K& key = i->first;

      stdutil::FastRemove(changed_, key);
      removed_.push_back(key);
      MarkChanged();
      return items_.erase(i);
   }

   core::Guard TraceMapChanges(const char* reason,
                             std::function<void(const K& k, const V& v)> added,
                             std::function<void(const K& k)> removed) const
   {
      ASSERT(added || removed);

      auto cb = [=]() {
         if (removed) {
            for (const auto& key : removed_) {
               removed(key);
            }
         }
         if (added) {
            for (const auto& key : changed_) {
               auto i = items_.find(key);
               ASSERT(i != items_.end());
               added(key, i->second);
            }
         }
      };
      return TraceObjectChanges(reason, cb);
   }

   // "add-or-update" operator, just like std::map and its ilk.
   V& operator[](const K& key) {
      ContainerType::iterator i = items_.find(key);
      if (i == items_.end()) {
         changed_.push_back(key);
         stdutil::FastRemove(removed_, key);
         i = items_.insert(std::make_pair(key, V())).first;
         MarkChanged();
      }
      return i->second;
   }
   
   void Remove(const K& key) {
      auto i = items_.find(key);
      if (i != items_.end()) {
         RemoveIterator(i);
      }
   }

   bool Contains(const K& key) const {
      return items_.find(key) != items_.end();
   }

   V Lookup(const K& key, V default) const {
      auto i = items_.find(key);
      return i != items_.end() ? i->second : default;
   }

   void Clear() {
      while (!items_.empty()) {
         Remove(items_.begin()->first);
      }
   }


   class LuaIterator {
   public:
      LuaIterator(const ContainerType& container) : container_(container) {
         i_ = container_.begin();
      }

      static int NextIteration(lua_State *L) {
         LuaIterator* iter = object_cast<LuaIterator*>(object(from_stack(L, -2)));
         return iter->Next(L);
      }

      int Next(lua_State *L) {
         if (i_ != container_.end()) {
            luabind::object(L, i_->first).push(L);
            luabind::object(L, i_->second).push(L);
            i_++;
            return 2;
         }
         return 0;
      }

   private:
      NO_COPY_CONSTRUCTOR(LuaIterator)

   private:
      const ContainerType&                   container_;
      typename ContainerType::const_iterator i_;
   };

   static void LuaIteratorStart(lua_State *L, const Map& s)
   {
      using namespace luabind;
      lua_pushcfunction(L, &LuaIterator::NextIteration); // f
      object(L, new LuaIterator(s.items_)).push(L); // s
      object(L, 1).push(L); // var (ignored)
   }

   class LuaPromise
   {
   public:
      LuaPromise(const Map& c, const char* reason) {
         auto changed = std::bind(&LuaPromise::OnChange, this, std::placeholders::_1, std::placeholders::_2);
         auto removed = std::bind(&LuaPromise::OnRemove, this, std::placeholders::_1);

         guard_ = c.TraceMapChanges(reason, changed, removed);
      }

      ~LuaPromise() {
         LOG(WARNING) << "killing map Promise...";
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

      void OnChange(const KeyType& key, const ValueType& value) {
         for (auto& cb : changedCbs_) {
            try {   
               luabind::call_function<void>(cb, key, value);
            } catch (std::exception const& e) {
               LOG(WARNING) << "lua error firing trace: " << e.what();
            }
         }
      }
      void OnRemove(const KeyType& key)  {
         for (auto& cb : removedCbs_) {
            try {
               luabind::call_function<void>(cb, key);
            } catch (std::exception const& e) {
               LOG(WARNING) << "lua error firing trace: " << e.what();
            }
         }
      }

   private:
      NO_COPY_CONSTRUCTOR(LuaPromise)

   private:
      core::Guard                     guard_;
      std::vector<luabind::object>  changedCbs_;
      std::vector<luabind::object>  removedCbs_;
   };

   LuaPromise* LuaTrace(const char* reason) const {
      return new LuaPromise(*this, reason);
   }

   static luabind::object LuaGet(struct lua_State* L, Map const& m, K const& k) {
      auto i = m.items_.find(k);
      if (i == m.items_.end()) {
         return luabind::object();
      }
      return luabind::object(L, i->second);
   }

   static luabind::scope RegisterLuaType(struct lua_State* L) {
      using namespace luabind;

      return
         lua::RegisterType<Map>()
            .def(tostring(const_self))
            .def("size",         &Map::GetSize)
            .def("is_empty",     &Map::IsEmpty)
            .def("get",          &Map::LuaGet)
            .def("items",        &Map::LuaIteratorStart)
            .def("trace",        &Map::LuaTrace)
         ,
         lua::RegisterType<LuaIterator>()
         ,
         lua::RegisterType<LuaPromise>()
            .def("on_added",     &LuaPromise::PushAddedCb)
            .def("on_removed",   &LuaPromise::PushRemovedCb)
            .def("destroy",      &LuaPromise::Destroy) // xxx: make this __gc!!
         ;
   }

public:
   void SaveValue(const Store& store, Protocol::Value* valmsg) const override {
      Protocol::Map::Update* msg = valmsg->MutableExtension(Protocol::Map::extension);
      for (const auto& key : changed_) {
         auto i = items_.find(key);
         ASSERT(i != items_.end());
         Protocol::Map::Entry* submsg = msg->add_added();
         SaveImpl<K>::SaveValue(store, submsg->mutable_key(), key);
         SaveImpl<V>::SaveValue(store, submsg->mutable_value(), i->second);
      }
      for (const auto& key : removed_) {
         SaveImpl<K>::SaveValue(store, msg->add_removed(), key);
      }
      changed_.clear();
      removed_.clear();
   }
   void LoadValue(const Store& store, const Protocol::Value& valmsg) override {
      const Protocol::Map::Update& msg = valmsg.GetExtension(Protocol::Map::extension);
      K key;
      V value;

      changed_.clear();
      removed_.clear();
      for (const Protocol::Map::Entry& entry : msg.added()) {
         SaveImpl<K>::LoadValue(store, entry.key(), key);
         SaveImpl<V>::LoadValue(store, entry.value(), value);
         changed_.push_back(key);
         items_[key] = value;
      }
      for (const Protocol::Value& removed : msg.removed()) {
         SaveImpl<K>::LoadValue(store, removed, key);
         auto i = items_.find(key);
         if (i != items_.end()) {
            removed_.push_back(key);
            items_.erase(i);
         }
      }
   }

   std::ostream& ToString(std::ostream& os) const {
      return (os << "(Map size:" << items_.size() << ")");
   }

private:
   NO_COPY_CONSTRUCTOR(Map);

private:
   ContainerType           items_;
   mutable std::vector<K>  changed_;
   mutable std::vector<K>  removed_;
};

template <typename K, typename V, typename H>
static std::ostream& operator<<(std::ostream& os, const Map<K, V, H>& in)
{
   return in.ToString(os);
}

END_RADIANT_DM_NAMESPACE
