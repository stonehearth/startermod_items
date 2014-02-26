#ifndef _RADIANT_DM_SAVE_IMPL_H
#define _RADIANT_DM_SAVE_IMPL_H

#include <memory>
#include "dbg_info.h"
#include "protocols/store.pb.h"

BEGIN_RADIANT_DM_NAMESPACE

class Store;

template<typename T>
struct SaveImpl
{
   static void SaveValue(Store const& store, SerializationType r, Protocol::Value* msg, const T& obj) {
      // A compile error here probably means you do not have the corrent
      // template specialization for your type.  See IMPLEMENT_DM_EXTENSION
      // below.
      obj.SaveValue(store, msg);
   }
   static void LoadValue(Store const& store, SerializationType r, Protocol::Value const& msg, T& obj) {
      obj.LoadValue(store, msg);
   }
   static void GetDbgInfo(T const& obj, DbgInfo &info) {
      obj.GetDbgInfo(info);
   }
};

// TODO: only for T's which are Objects!
template <class T>
struct SaveImpl<std::shared_ptr<T>>
{
   static void SaveValue(Store const& store, SerializationType r, Protocol::Value* msg, const std::shared_ptr<T>& value) {
      ObjectId id = value ? value->GetObjectId() : 0;
      msg->SetExtension(Protocol::Ref::ref_object_id, id);
   }
   static void LoadValue(Store const& store, SerializationType r, Protocol::Value const& msg, std::shared_ptr<T>& value) {
      ObjectId id = msg.GetExtension(Protocol::Ref::ref_object_id);
      value = store.FetchObject<T>(id);
   }
   static void GetDbgInfo(std::shared_ptr<T> obj, DbgInfo &info) {
      if (obj) {
         info.os << "[shared_ptr ";
         SaveImpl<T>::GetDbgInfo(*obj, info);
         info.os << "]";
      } else {
         info.os << "[shared_ptr nullptr]";
      }
   }
};

// TODO: only for T's which are Objects!
template <class T>
struct SaveImpl<std::weak_ptr<T>>
{
   static void SaveValue(Store const& store, SerializationType r, Protocol::Value* msg, const std::weak_ptr<T>& value) {
      SaveImpl<std::shared_ptr<T>>().SaveValue(store, r, msg, value.lock());
   }
   static void LoadValue(Store const& store, SerializationType r, Protocol::Value const& msg, std::weak_ptr<T>& value) {
      ObjectId id = msg.GetExtension(Protocol::Ref::ref_object_id);
      value = store.FetchObject<T>(id);
   }
   static void GetDbgInfo(std::weak_ptr<T> o, DbgInfo &info) {
      auto obj = o.lock();
      if (obj) {
         info.os << "[weak_ptr ";
         SaveImpl<T>::GetDbgInfo(*obj, info);
         info.os << "]";
      } else {
         info.os << "[weak_ptr nullptr]";
      }
   }
};

#define IMPLEMENT_DM_EXTENSION(T, E) \
template<> \
struct ::radiant::dm::SaveImpl<T> { \
   static void SaveValue(Store const& store, SerializationType r, Protocol::Value* msg, const T& obj) { \
      obj.SaveValue(msg->MutableExtension(E)); \
   } \
   static void LoadValue(Store const& store, SerializationType r, Protocol::Value const& msg, T& obj) { \
      obj.LoadValue(msg.GetExtension(E)); \
   } \
   static void GetDbgInfo(T const& obj, DbgInfo &info) { \
      info.os << obj; \
   } \
};

#define IMPLEMENT_DM_BASIC_TYPE(T, E) \
template<> \
struct ::radiant::dm::SaveImpl<T> \
{ \
   static void SaveValue(Store const& store, SerializationType r, Protocol::Value* msg, const T& value) { \
      msg->SetExtension(E, value); \
   } \
   static void LoadValue(Store const& store, SerializationType r, Protocol::Value const& msg, T& value) { \
      value = msg.GetExtension(E); \
   } \
   static void GetDbgInfo(T const& obj, DbgInfo &info) { \
      info.os << obj; \
   } \
};

#define IMPLEMENT_DM_ENUM(T) \
template<> \
struct ::radiant::dm::SaveImpl<T> \
{ \
   static void SaveValue(Store const& store, SerializationType r, Protocol::Value* msg, const T& value) { \
      msg->SetExtension(Protocol::integer, (int)value); \
   } \
   static void LoadValue(Store const& store, SerializationType r, Protocol::Value const& msg, T& value) { \
      value = (T)msg.GetExtension(Protocol::integer); \
   } \
   static void GetDbgInfo(T const& obj, DbgInfo &info) { \
      info.os << obj; \
   } \
};

#define IMPLEMENT_DM_NOP(T) \
template<> \
struct ::radiant::dm::SaveImpl<T> \
{ \
   static void SaveValue(Store const& store, SerializationType r, Protocol::Value* msg, T value) { \
   } \
   static void LoadValue(Store const& store, SerializationType r, Protocol::Value const& msg, T value) { \
   } \
   static void GetDbgInfo(T const& obj, DbgInfo &info) { \
   } \
};

END_RADIANT_DM_NAMESPACE

#endif //  _RADIANT_DM_SAVE_IMPL_H
