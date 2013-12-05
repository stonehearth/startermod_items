#ifndef _RADIANT_OM_LUA_REGION_H
#define _RADIANT_OM_LUA_REGION_H

#include "om/region.h"

BEGIN_RADIANT_OM_NAMESPACE

class LuaDeepRegionGuard : public std::enable_shared_from_this<LuaDeepRegionGuard>
{
public:
   LuaDeepRegionGuard(DeepRegionGuardPtr trace) :
      trace_(trace)
   {
   }

   std::shared_ptr<LuaDeepRegionGuard> OnChanged(luabind::object changed_cb)
   {
      if (!trace_) {
         throw std::logic_error("called on_changed on invalid trace");
      }
      trace_->OnChanged([this, changed_cb](csg::Region3 const& value) {
         try {
            luabind::call_function<void>(changed_cb, value);
         } catch (std::exception const& e) {
            LOG(WARNING) << "exception delivering lua trace: " << e.what();
         }
      });
      return shared_from_this();
   }

   std::shared_ptr<LuaDeepRegionGuard> PushObjectState()
   {
      if (!trace_) {
         throw std::logic_error("called push_object_state on invalid trace");
      }
      trace_->PushObjectState();
      return shared_from_this();
   }

   void Destroy()
   {
      trace_ = nullptr;
   }
   
private:
   DeepRegionGuardPtr   trace_;
};

DECLARE_SHARED_POINTER_TYPES(LuaDeepRegionGuard)

END_RADIANT_OM_NAMESPACE

#endif //  _RADIANT_OM_REGION_H
