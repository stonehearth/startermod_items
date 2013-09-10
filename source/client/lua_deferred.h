#ifndef _RADIANT_CLIENT_LUA_DEFERRED_H
#define _RADIANT_CLIENT_LUA_DEFERRED_H

#include "radiant_net.h"
#include "radiant_json.h"
#include "lua/script_host.h"
#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

template <typename Deferred, typename Derived>
class LuaDeferred
{
public:
   LuaDeferred(lua::ScriptHost* sh, std::shared_ptr<Deferred> d) : scriptHost_(sh), deferred_(d) {}
   virtual ~LuaDeferred() {
      LOG(WARNING) << "deferred going away!";
   }

public: // the lua promise...
   static std::shared_ptr<Derived> LuaFail(std::shared_ptr<Derived> obj, luabind::object cb) {
      obj->deferred_->Fail([=](JSONNode const& node) {
         obj->scriptHost_->CallFunction<luabind::object>(cb, node);
      });
      return obj;
   }
   static std::shared_ptr<Derived> LuaAlways(std::shared_ptr<Derived> obj, luabind::object cb) {
      obj->deferred_->Always([=]() {
         obj->scriptHost_->CallFunction<luabind::object>(cb);
      });
      return obj;
   }
protected:
   lua::ScriptHost*           scriptHost_;
   std::shared_ptr<Deferred>  deferred_;
};

template <typename Deferred>
class LuaDeferred1 : public LuaDeferred<Deferred, LuaDeferred1<Deferred>>
{
public:
   typedef std::function<luabind::object(typename Deferred::Type0 const&)> EncodeFn;

   LuaDeferred1(lua::ScriptHost* sh, std::shared_ptr<Deferred> d) : LuaDeferred(sh, d) { }
   LuaDeferred1(lua::ScriptHost* sh, std::shared_ptr<Deferred> d, EncodeFn encode) : LuaDeferred(sh, d), encode_(encode) { }

public: // the lua promise...
   static std::shared_ptr<LuaDeferred1> LuaDone(std::shared_ptr<LuaDeferred1> obj, luabind::object cb) {
      obj->deferred_->Done([=](typename Deferred::Type0 const& r) {
         obj->scriptHost_->CallFunction<luabind::object>(cb, r);
      });
      return obj;
   }
   static std::shared_ptr<LuaDeferred1> LuaProgress(std::shared_ptr<LuaDeferred1> obj, luabind::object cb) {
      obj->deferred_->Progress([=](typename Deferred::Type0 const& r) {
         if (obj->encode_) {
            obj->scriptHost_->CallFunction<luabind::object>(cb, obj->encode_(r));
         } else {
            obj->scriptHost_->CallFunction<luabind::object>(cb, r);
         }
      });
      return obj;
   }

public:
   EncodeFn encode_;
};

template <typename Deferred>
class LuaDeferred2 : public LuaDeferred<Deferred, LuaDeferred2<Deferred>>
{
public:
   LuaDeferred2(lua::ScriptHost* sh, std::shared_ptr<Deferred> d) : LuaDeferred(sh, d) { }

public:  // the lua interface to the promise...
   static std::shared_ptr<LuaDeferred2> LuaDone(std::shared_ptr<LuaDeferred2> obj, luabind::object cb) {
      obj->deferred_->Done([=](Deferred::Type0 const& r, Deferred::Type1 const& s) {
         obj->scriptHost_->CallFunction<luabind::object>(cb, r, s);
      });
      return obj;
   }
   static std::shared_ptr<LuaDeferred2> LuaProgress(std::shared_ptr<LuaDeferred2> obj, luabind::object cb) {
      obj->deferred_->Progress([=](Deferred::Type0 const& r, Deferred::Type1 const& s) {
         obj->scriptHost_->CallFunction<luabind::object>(cb, r, s);
      });
      return obj;
   }
};

class LuaResponse
{
public:
   LuaResponse(lua::ScriptHost* sh, std::shared_ptr<net::IResponse> response) : scriptHost_(sh), response_(response), finished_(false) { }
   ~LuaResponse() {
      if (!finished_) {
         response_->SetResponse(200);
      }
   }
   void Complete(luabind::object obj) {
      ASSERT(!finished_); // If this fires, the client is trying to completely multiple times!
      if (!finished_) {
         finished_ = true;
         response_->SetResponse(200, scriptHost_->LuaToJson(obj), "application/json");
      }
   }

private:
   bool                            finished_;
   lua::ScriptHost*                scriptHost_;
   std::shared_ptr<net::IResponse> response_;
};


END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_LUA_DEFERRED_H
