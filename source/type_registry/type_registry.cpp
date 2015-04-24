#include <type_traits>
#include "radiant.h"
#include "radiant_macros.h"
#include "type_registry.h"
#include "lib/typeinfo/register_type.h"
#include "om/region.h"
#include "om/all_objects.h"
#include "om/all_components.h"
#include "om/components/data_store_ref_wrapper.h"
#include "csg/region.h"
#include "csg/ray.h"
#include "csg/color.h"
#include "csg/random_number_generator.h"
#include "lib/lua/data_object.h"
#include "dm/lua_types.h"

using namespace radiant;

// xxx: remove this once dm_save_impl.h has been purged.
#include "dm/types/all_loader_types.h"

/*
 * The table of all the types in the system.
 */
#define ALL_OBJECT_TYPES \
   OBJECT_TYPE(int,                            1,                                   RegisterNotImplementedType) \
   OBJECT_TYPE(float,                          2,                                   RegisterNotImplementedType) \
   OBJECT_TYPE(double,                         3,                                   RegisterNotImplementedType) \
   OBJECT_TYPE(bool,                           4,                                   RegisterNotImplementedType) \
   OBJECT_TYPE(std::string,                    5,                                   RegisterNotImplementedType) \
   OBJECT_TYPE(core::StaticString,             6,                                   RegisterNotImplementedType) \
   \
   OBJECT_TYPE(csg::Cube3,                   100,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Cube3f,                  101,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Rect2,                   102,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Rect2f,                  103,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Line1,                   104,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Line1f,                  105,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region3,                 106,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region3f,                107,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region2,                 108,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region2f,                109,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region1,                 110,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region1f,                111,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point1,                  112,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point1f,                 113,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point2,                  114,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point2f,                 115,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point3,                  116,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point3f,                 117,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Transform,               118,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Quaternion,              119,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Ray3,                    120,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Color3,                  121,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Color4,                  122,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::RandomNumberGenerator,   123,                                   RegisterValueTypeLegacy) \
   \
   OBJECT_TYPE(om::ClockRef,                 om::Clock::DmType,                     RegisterGameObjectType)  \
   OBJECT_TYPE(om::EntityContainerRef,       om::EntityContainer::DmType,           RegisterGameObjectType) \
   OBJECT_TYPE(om::MobRef,                   om::Mob::DmType,                       RegisterGameObjectType) \
   OBJECT_TYPE(om::ModelVariantsRef,         om::ModelVariants::DmType,             RegisterGameObjectType) \
   OBJECT_TYPE(om::RegionCollisionShapeRef,  om::RegionCollisionShape::DmType,      RegisterGameObjectType) \
   OBJECT_TYPE(om::MovementModifierShapeRef, om::MovementModifierShape::DmType,     RegisterGameObjectType) \
   OBJECT_TYPE(om::MovementGuardShapeRef,    om::MovementGuardShape::DmType,        RegisterGameObjectType) \
   OBJECT_TYPE(om::TerrainRef,               om::Terrain::DmType,                   RegisterGameObjectType) \
   OBJECT_TYPE(om::VerticalPathingRegionRef, om::VerticalPathingRegion::DmType,     RegisterGameObjectType) \
   OBJECT_TYPE(om::EffectListRef,            om::EffectList::DmType,                RegisterGameObjectType) \
   OBJECT_TYPE(om::RenderInfoRef,            om::RenderInfo::DmType,                RegisterGameObjectType) \
   OBJECT_TYPE(om::SensorListRef,            om::SensorList::DmType,                RegisterGameObjectType) \
   OBJECT_TYPE(om::DestinationRef,           om::Destination::DmType,               RegisterGameObjectType) \
   OBJECT_TYPE(om::UnitInfoRef,              om::UnitInfo::DmType,                  RegisterGameObjectType) \
   OBJECT_TYPE(om::ItemRef,                  om::Item::DmType,                      RegisterGameObjectType) \
   OBJECT_TYPE(om::ModListRef,               om::ModList::DmType,                   RegisterGameObjectType) \
   OBJECT_TYPE(om::EntityRef,                om::Entity::DmType,                    RegisterGameObjectType) \
   OBJECT_TYPE(om::EffectPtr,                om::Effect::DmType,                    RegisterGameObjectType) \
   OBJECT_TYPE(om::SensorRef,                om::Sensor::DmType,                    RegisterGameObjectType) \
   OBJECT_TYPE(om::ModelLayerPtr,            om::ModelLayer::DmType,                RegisterGameObjectType) \
   OBJECT_TYPE(om::ErrorBrowserPtr,          om::ErrorBrowser::DmType,              RegisterGameObjectType) \
   OBJECT_TYPE(om::Region2BoxedPtr,          om::Region2Boxed::DmType,              RegisterGameObjectType) \
   OBJECT_TYPE(om::Region3BoxedPtr,          om::Region3Boxed::DmType,              RegisterGameObjectType) \
   OBJECT_TYPE(om::Region2fBoxedPtr,         om::Region2fBoxed::DmType,             RegisterGameObjectType) \
   OBJECT_TYPE(om::Region3fBoxedPtr,         om::Region3fBoxed::DmType,             RegisterGameObjectType) \
   OBJECT_TYPE(om::Selection,                200,                                   RegisterValueTypeLegacy) \
   \
   OBJECT_TYPE(lua::DataObjectPtr,           301,                                   RegisterNotImplementedType) \
   \
   OBJECT_TYPE(dm::NumberMapPtr,             402,                                   RegisterNotImplementedType) \
   \


// Evaluate whether or not a particular type is stored in radiant::dm as a dynamic type.
// "dynamic" types are ones which were allocated on the heap.  
template <typename T> struct IsDynamicObject : std::false_type {};
// xxx: hm.  these look more accurate, but don't compile.  why not?
// template <typename T> struct IsDynamicObject<std::weak_ptr<T>> : std::is_base_of<dm::Object, T>::value {};
// template <typename T> struct IsDynamicObject<std::shared_ptr<T>> : std::is_base_of<dm::Object, T>::value {};
template <typename T> struct IsDynamicObject<std::weak_ptr<T>> : std::true_type{};
template <typename T> struct IsDynamicObject<std::shared_ptr<T>> : std::true_type{};

template <typename T> std::shared_ptr<T> ConvertToSharedPtr(std::shared_ptr<T> obj) { return obj; }
template <typename T> std::shared_ptr<T> ConvertToSharedPtr(std::weak_ptr<T> o) { return o.lock(); }

// Given a std::shared_ptr<T> or std::weak_ptr<T>, evaluate to T
template <typename T> struct PointerType;
template <typename T> struct PointerType<std::shared_ptr<T>> { typedef T value; };
template <typename T> struct PointerType<std::weak_ptr<T>> { typedef T value; };

// We would like to write this is:
//
//    template <typename T, class = std::enable_if<IsDynamicObject<T>>::type >
//    void RegisterGameObject()
//
// But VS 2012 doesn't support default template parameters on functions (though c++0x allows it).
// For now, fake this out with the return value. =(
template <typename T>
typename std::enable_if<IsDynamicObject<T>::value, void>::type
RegisterGameObjectType()
{
   // Game Object Conversions
   //
   //   lua <-> proto - serialize the id on save and pull it out of the remote store on load.
   //                   actual object remoting is done by the dm layer.   
   //
   typeinfo::RegisterType<T>()
      .SetLuaToProtobuf([](dm::Store const& store, luabind::object const& lua, Protocol::Value* msg, int flags) {
         // cast the lua object to the appropriate cpp type.  this will either be a shared_ptr or
         // a weak_ptr to the actual dm::Object derived cpp class.  After we've gotten it, convert it to
         // a shared pointer
         T o = luabind::object_cast<T>(lua);
         auto obj = ConvertToSharedPtr(o);

         // send the id of the game object, or 0 if the pointer ended up being nullptr.
         msg->set_type_id(typeinfo::Type<T>::id);
         if (obj) {
            msg->SetExtension(Protocol::Ref::ref_object_id, obj->GetObjectId());
         } else {
            msg->SetExtension(Protocol::Ref::ref_object_id, 0);
         }
      })
      .SetProtobufToLua([](dm::Store const& store, Protocol::Value const& msg, luabind::object& lua, int flags) {
         // get the type_id and object_id of the game object and verify they're what we expect.
         typeinfo::TypeId typeId = msg.type_id();
         dm::ObjectId objectId = msg.GetExtension(Protocol::Ref::ref_object_id);
         ASSERT(typeId == typeinfo::Type<T>::id);

         // pull the game object out of the store and write it into the `lua` out parameter.  T here
         // could be a shared or weak pointer, so we do this in two steps to ensure the luabind object
         // constructor gets the correct type information (FetchObject always returns a shared_ptr).
         T obj = store.FetchObject<PointerType<T>::value>(objectId);
         lua = luabind::object(store.GetInterpreter(), obj);
      });
}


template<>
struct dm::SaveImpl<csg::RandomNumberGenerator> {
   static void SaveValue(const dm::Store& store, dm::SerializationType r, Protocol::Value* msg, csg::RandomNumberGenerator const& obj) {
      Protocol::RandomNumberGenerator* rng_msg = msg->MutableExtension(Protocol::RandomNumberGenerator::extension);
      rng_msg->set_state(BUILD_STRING(obj));
   }
   static void LoadValue(const Store& store, dm::SerializationType r, const Protocol::Value& msg, csg::RandomNumberGenerator& obj) {
      Protocol::RandomNumberGenerator const& rng_msg = msg.GetExtension(Protocol::RandomNumberGenerator::extension);
      std::istringstream is(rng_msg.state());
      is >> obj;
   }
   static void GetDbgInfo(lua::DataObject const& obj, dm::DbgInfo &info) {
      info.os << "[lua_object]";
   }
};

template <typename T>
void
RegisterValueTypeLegacy()
{
   // Value Conversions
   //
   //   lua <-> proto - serialize the actual value
   //
   typeinfo::RegisterType<T>()
      .SetLuaToProtobuf([](dm::Store const& store, luabind::object const& lua, Protocol::Value* msg, int flags) {
         T value = luabind::object_cast<T>(lua);
         msg->set_type_id(typeinfo::Type<T>::id);
         dm::SaveImpl<T>::SaveValue(store, dm::PERSISTANCE, msg, value);
      })
      .SetProtobufToLua([](dm::Store const& store, Protocol::Value const& msg, luabind::object& lua, int flags) {
         typeinfo::TypeId typeId = msg.type_id();
         ASSERT(typeId == typeinfo::Type<T>::id);
         T value;
         dm::SaveImpl<T>::LoadValue(store, dm::PERSISTANCE, msg, value);
         lua = luabind::object(store.GetInterpreter(), value);
      });
}

template <typename T>
void RegisterNotImplementedType()
{
   typeinfo::RegisterType<T>()
      .SetLuaToProtobuf([](dm::Store const& store, luabind::object const& lua, Protocol::Value* msg, int flags) {
         NOT_YET_IMPLEMENTED();
      })
      .SetProtobufToLua([](dm::Store const& store, Protocol::Value const& msg, luabind::object& lua, int flags) {
         NOT_YET_IMPLEMENTED();
      });
}

#define OBJECT_TYPE(T, ID, REG) DECLARE_TYPEINFO(T, ID)
   ALL_OBJECT_TYPES
#undef OBJECT_TYPE

DECLARE_TYPEINFO(om::DataStorePtr, om::DataStore::DmType)
DECLARE_TYPEINFO(om::DataStoreRefWrapper, 10000)

void TypeRegistry::Initialize()
{
#define OBJECT_TYPE(T, ID, REG) REG<T>();
   ALL_OBJECT_TYPES
#undef OBJECT_TYPE

   // Treat datastores differently from other objects.  On the server we have either cpp managed or
   // unmanaged datastores.  Cpp managed datastores are kept alive only by C++ objects.  They are exposed
   // to lua as om::DataStoreRefWrapper, which is a lightweight wrapper around om::DataStoreRef to prevent
   // luabind from doing type cohersion.  The moment the last cpp reference goes away, all the lua references
   // become invalid.  Unmanaged datastores are just om::DataStorePtr's everywhere, so they stick around
   // until the last cpp reference goes away and the last lua reference get's GC'ed.
   //
   // Regardless of all that, we want to exposed ALL datastores on the client as om::DataStoreRef's, since
   // their lifetime is strictly determined by the lifetime of the corresponding object on the server.
   //
   // This leads to the following rules:
   //   1) When saving to disk, make sure we preserve the type going in.
   //   2) When remoting over the network, make sure we convert the type to an om::DataStoreRef
   //
   // You could argue this methodology should apply to ALL std::shared_ptr<> objects on the server and
   // not just DataStore's, but let's try it out with DataStore's first and see how it goes.  If we're
   // seeing big leaks of other shared_ptr based server objects and DataStore's go well, definitely convert
   // everything over.  -- tony
   //
   typeinfo::RegisterType<om::DataStorePtr>()
      .SetLuaToProtobuf([](dm::Store const& store, luabind::object const& lua, Protocol::Value* msg, int flags) {
         om::DataStorePtr obj = luabind::object_cast<om::DataStorePtr>(lua);
         msg->set_type_id(typeinfo::Type<om::DataStorePtr>::id);
         msg->SetExtension(Protocol::Ref::ref_object_id, obj ? obj->GetObjectId() : 0);
      })
      .SetProtobufToLua([](dm::Store const& store, Protocol::Value const& msg, luabind::object& lua, int flags) {
         typeinfo::TypeId typeId = msg.type_id();
         dm::ObjectId objectId = msg.GetExtension(Protocol::Ref::ref_object_id);
         ASSERT(typeId == typeinfo::Type<om::DataStorePtr>::id);

         om::DataStorePtr obj = store.FetchObject<om::DataStore>(objectId);
         if ((flags & marshall::Convert::Flags::REMOTE) != 0) {
            // Remoting!  That's case #2 in the big comment above.  Regardless of how it lives on the
            // server, convert the DataStore to a DataStoreRef.
            lua = luabind::object(store.GetInterpreter(), om::DataStoreRefWrapper(obj));
         } else {
            // Reestoring from disk!  That's case #1 above.  Preserve the type, which is om::DataStorePtr.
            lua = luabind::object(store.GetInterpreter(), obj);
         }
      });

   typeinfo::RegisterType<om::DataStoreRefWrapper>()
      .SetLuaToProtobuf([](dm::Store const& store, luabind::object const& lua, Protocol::Value* msg, int flags) {
         om::DataStoreRefWrapper ref = luabind::object_cast<om::DataStoreRefWrapper>(lua);
         om::DataStorePtr obj = ref.lock();
         msg->set_type_id(typeinfo::Type<om::DataStoreRefWrapper>::id);
         msg->SetExtension(Protocol::Ref::ref_object_id, obj ? obj->GetObjectId() : 0);
      })
      .SetProtobufToLua([](dm::Store const& store, Protocol::Value const& msg, luabind::object& lua, int flags) {
         typeinfo::TypeId typeId = msg.type_id();
         dm::ObjectId objectId = msg.GetExtension(Protocol::Ref::ref_object_id);
         ASSERT(typeId == typeinfo::Type<om::DataStoreRefWrapper>::id);

         // Read the big comment above... Cases #1 and #2 are the same: we need a ref here.  So get one!
         om::DataStorePtr obj = store.FetchObject<om::DataStore>(objectId);
         lua = luabind::object(store.GetInterpreter(), om::DataStoreRefWrapper(obj));
      });

}
