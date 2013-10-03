#include "pch.h"
#include "radiant.h"
#include "radiant_macros.h"
#include "lua/register.h"
#include "lua/script_host.h"
#include "lua_destination_component.h"
#include "om/components/destination.h"
#include "dm/guard.h"

using namespace ::luabind;
using namespace ::radiant;
using namespace ::radiant::om;

class LuaRegionTrace;
DECLARE_SHARED_POINTER_TYPES(LuaRegionTrace);

class LuaRegionTrace : public std::enable_shared_from_this<LuaRegionTrace>
{
   public:
      LuaRegionTrace(lua_State* L, dm::Boxed<BoxedRegion3Ptr> const& bbrp, const char* reason) {
         L_ = lua::ScriptHost::GetCallbackThread(L);
         region_guard_ = TraceBoxedRegion3PtrFieldVoid(bbrp, reason, [=]{
            FireTrace();
         });
      }

   public:
      LuaRegionTracePtr OnChanged(object cb) {
         cbs_.push_back(object(L_, cb));
         return shared_from_this();
      }

      void Destroy() {
         region_guard_ = nullptr;
         cbs_.clear();
      }

      void FireTrace() {
         for (const auto& cb : cbs_) {
            try {
               call_function<void>(cb);
            } catch (std::exception &e) {
               LOG(WARNING) << "error in lua callback: " << e.what();
            }
         }
      }

   private:
      lua_State                  *L_;
      BoxedRegionGuardPtr        region_guard_;
      std::vector<object>        cbs_;
};

IMPLEMENT_TRIVIAL_TOSTRING(LuaRegionTrace)

LuaRegionTracePtr Destination_TraceRegion(lua_State* L, Destination const& d, const char* reason)
{
   return std::make_shared<LuaRegionTrace>(L, d.GetRegion(), reason);
}

BoxedRegion3Ptr Destination_GetRegion(Destination const& d)
{
   return *d.GetRegion();
}

DestinationRef Destination_SetRegion(DestinationRef d, BoxedRegion3Ptr r)
{
   DestinationPtr destination = d.lock();
   if (destination) {
      destination->SetRegion(r);
   }
   return d;
}

scope LuaDestinationComponent::RegisterLuaTypes(lua_State* L)
{
   return
      lua::RegisterDerivedObject<Destination, Component>()
         .def("get_region",               &Destination_GetRegion)
         .def("set_region",               &Destination_SetRegion)
         .def("trace_region",             &Destination_TraceRegion)
         .def("set_auto_update_adjacent", &Destination::SetAutoUpdateAdjacent)
      ,
      lua::RegisterTypePtr<LuaRegionTrace>()
         .def("on_changed",               &LuaRegionTrace::OnChanged)
         .def("destroy",                  &LuaRegionTrace::Destroy)
      ;
}
