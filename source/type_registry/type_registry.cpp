#include <type_traits>
#include "radiant.h"
#include "radiant_macros.h"
#include "type_registry.h"
#include "lib/typeinfo/register_type.h"
#include "om/region.h"
#include "om/all_objects.h"
#include "om/all_components.h"
#include "csg/region.h"
#include "csg/ray.h"
#include "csg/color.h"
#include "csg/random_number_generator.h"
#include "lib/lua/data_object.h"

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
   \
   OBJECT_TYPE(csg::Cube3,                   100,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Cube3f,                  101,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Rect2,                   102,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Rect2f,                  103,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Line1,                   104,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region3,                 105,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region3f,                106,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region2,                 107,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region2f,                108,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Region1,                 109,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point1,                  110,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point1f,                 111,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point2,                  112,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point2f,                 113,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point3,                  114,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Point3f,                 115,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Transform,               116,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Quaternion,              117,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Ray3,                    118,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Color3,                  119,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::Color4,                  120,                                   RegisterValueTypeLegacy) \
   OBJECT_TYPE(csg::RandomNumberGenerator,   121,                                   RegisterValueTypeLegacy) \
   \
   OBJECT_TYPE(om::ClockRef,                 om::Clock::DmType,                     RegisterGameObjectType)  \
   OBJECT_TYPE(om::EntityContainerRef,       om::EntityContainer::DmType,           RegisterGameObjectType) \
   OBJECT_TYPE(om::MobRef,                   om::Mob::DmType,                       RegisterGameObjectType) \
   OBJECT_TYPE(om::ModelVariantsRef,         om::ModelVariants::DmType,             RegisterGameObjectType) \
   OBJECT_TYPE(om::RegionCollisionShapeRef,  om::RegionCollisionShape::DmType,      RegisterGameObjectType) \
   OBJECT_TYPE(om::TerrainRef,               om::Terrain::DmType,                   RegisterGameObjectType) \
   OBJECT_TYPE(om::VerticalPathingRegionRef, om::VerticalPathingRegion::DmType,     RegisterGameObjectType) \
   OBJECT_TYPE(om::EffectListRef,            om::EffectList::DmType,                RegisterGameObjectType) \
   OBJECT_TYPE(om::RenderInfoRef,            om::RenderInfo::DmType,                RegisterGameObjectType) \
   OBJECT_TYPE(om::SensorListRef,            om::SensorList::DmType,                RegisterGameObjectType) \
   OBJECT_TYPE(om::TargetTablesRef,          om::TargetTables::DmType,              RegisterGameObjectType) \
   OBJECT_TYPE(om::DestinationRef,           om::Destination::DmType,               RegisterGameObjectType) \
   OBJECT_TYPE(om::UnitInfoRef,              om::UnitInfo::DmType,                  RegisterGameObjectType) \
   OBJECT_TYPE(om::ItemRef,                  om::Item::DmType,                      RegisterGameObjectType) \
   OBJECT_TYPE(om::ModListRef,               om::ModList::DmType,                   RegisterGameObjectType) \
   OBJECT_TYPE(om::EntityRef,                om::Entity::DmType,                    RegisterGameObjectType) \
   OBJECT_TYPE(om::EffectPtr,                om::Effect::DmType,                    RegisterGameObjectType) \
   OBJECT_TYPE(om::SensorRef,                om::Sensor::DmType,                    RegisterGameObjectType) \
   OBJECT_TYPE(om::TargetTablePtr,           om::TargetTable::DmType,               RegisterGameObjectType) \
   OBJECT_TYPE(om::TargetTableGroupPtr,      om::TargetTableGroup::DmType,          RegisterGameObjectType) \
   OBJECT_TYPE(om::TargetTableEntryPtr,      om::TargetTableEntry::DmType,          RegisterGameObjectType) \
   OBJECT_TYPE(om::DataStorePtr,             om::DataStore::DmType,                 RegisterGameObjectType) \
   OBJECT_TYPE(om::ModelLayerPtr,            om::ModelLayer::DmType,                RegisterGameObjectType) \
   OBJECT_TYPE(om::ErrorBrowserPtr,          om::ErrorBrowser::DmType,              RegisterGameObjectType) \
   OBJECT_TYPE(om::Region2BoxedPtr,          om::Region2Boxed::DmType,              RegisterGameObjectType) \
   OBJECT_TYPE(om::Region3BoxedPtr,          om::Region3Boxed::DmType,              RegisterGameObjectType) \
   OBJECT_TYPE(om::Selection,                200,                                   RegisterValueTypeLegacy) \
   \
   OBJECT_TYPE(lua::DataObjectPtr,           301,                                   RegisterNotImplementedType) \


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

void TypeRegistry::Initialize()
{
#define OBJECT_TYPE(T, ID, REG) REG<T>();
   ALL_OBJECT_TYPES
#undef OBJECT_TYPE
}
