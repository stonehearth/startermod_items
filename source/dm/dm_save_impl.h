#ifndef _RADIANT_DM_SAVE_IMPL_H
#define _RADIANT_DM_SAVE_IMPL_H

#include "dm.h"
#include "dbg_info.h"
#include <memory>

BEGIN_RADIANT_DM_NAMESPACE

class Store;

template<typename T>
struct SaveImpl
{
   static void SaveValue(const Store& store, Protocol::Value* msg, const T& obj) {
      // A compile error here probably means you do not have the corrent
      // template specialization for your type.  See IMPLEMENT_DM_EXTENSION
      // below.
      obj.SaveValue(store, msg);
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, T& obj) {
      obj.LoadValue(store, msg);
   }
   static void GetDbgInfo(T const& obj, DbgInfo &info) {
      obj.GetDbgInfo(info);
   }
};

#define IMPLEMENT_DM_EXTENSION(T, E) \
template<> \
struct ::radiant::dm::SaveImpl<T> { \
   static void SaveValue(const Store& store, Protocol::Value* msg, const T& obj) { \
      obj.SaveValue(msg->MutableExtension(E)); \
   } \
   static void LoadValue(const Store& store, const Protocol::Value& msg, T& obj) { \
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
   static void SaveValue(const Store& store, Protocol::Value* msg, const T& value) { \
      msg->SetExtension(E, value); \
   } \
   static void LoadValue(const Store& store, const Protocol::Value& msg, T& value) { \
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
   static void SaveValue(const Store& store, Protocol::Value* msg, T value) { \
      msg->SetExtension(Protocol::integer, (int)value); \
   } \
   static void LoadValue(const Store& store, const Protocol::Value& msg, T value) { \
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
   static void SaveValue(const Store& store, Protocol::Value* msg, T value) { \
   } \
   static void LoadValue(const Store& store, const Protocol::Value& msg, T value) { \
   } \
   static void GetDbgInfo(T const& obj, DbgInfo &info) { \
   } \
};

IMPLEMENT_DM_BASIC_TYPE(int,  Protocol::integer);
IMPLEMENT_DM_BASIC_TYPE(bool, Protocol::boolean);
IMPLEMENT_DM_BASIC_TYPE(float, Protocol::floatingpoint);
IMPLEMENT_DM_BASIC_TYPE(std::string, Protocol::string);

END_RADIANT_DM_NAMESPACE

#endif //  _RADIANT_DM_SAVE_IMPL_H
