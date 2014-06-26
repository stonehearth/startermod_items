local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local BuildCallHandler = class()

-- these are quite annoying.  we can get rid of them by implementing and using
-- LuaToProto <-> ProtoToLua in the RPC layer (see lib/typeinfo/dispatcher.h)
local function ToPoint3(pt)
   return Point3(pt.x, pt.y, pt.z)
end
local function ToCube3(box)
   return Cube3(ToPoint3(box.min), ToPoint3(box.max))
end

-- set whether or not the specified building should be worked on.
function BuildCallHandler:set_building_active(session, request, building, enabled)
   stonehearth.build:set_active(building, enabled)
   return { success = true }
end

function BuildCallHandler:set_building_teardown(session, request, building, enabled)
   stonehearth.build:set_teardown(building, enabled)
   return { success = true }
end

function BuildCallHandler:get_service(session, request, name)
   if stonehearth[name] then
      -- we'd like to just send the store address rather than the actual
      -- store, but there's no way for the client to receive a store
      -- address and *not* automatically convert it back!
      return stonehearth[name].__saved_variables
   end
   request:fail('no such service')
end

function BuildCallHandler:get_client_service(session, request, name)
   if stonehearth[name] then
      -- we'd like to just send the store address rather than the actual
      -- store, but there's no way for the client to receive a store
      -- address and *not* automatically convert it back!
      return stonehearth[name].__saved_variables
   end
   request:fail('no such service')
end

return BuildCallHandler
