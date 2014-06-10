local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local BuildCallHandler = class()
local BuildEditor = require 'services.server.build.build_editor'

-- these are quite annoying.  we can get rid of them by implementing and using
-- LuaToProto <-> ProtoToLua in the RPC layer (see lib/typeinfo/dispatcher.h)
local function ToPoint3(pt)
   return Point3(pt.x, pt.y, pt.z)
end
local function ToCube3(box)
   return Cube3(ToPoint3(box.min), ToPoint3(box.max))
end

local build_editor
function BuildCallHandler:get_build_editor(session, request)
   if not build_editor then
      build_editor = BuildEditor()
   end
   return { build_editor = build_editor:get_model() }
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

function BuildCallHandler:add_floor(session, request, floor_uri, box, brush_shape)
   stonehearth.build:add_floor(session, request, floor_uri, ToCube3(box), brush_shape)
end

function BuildCallHandler:add_wall(session, request, columns_uri, walls_uri, p0, p1, normal)
   stonehearth.build:add_wall(session, request, columns_uri, walls_uri, ToPoint3(p0), ToPoint3(p1), ToPoint3(normal))
end

function BuildCallHandler:grow_walls(session, request, building, columns_uri, walls_uri)
   stonehearth.build:grow_walls(session, request, building, columns_uri, walls_uri)
end

function BuildCallHandler:grow_roof(session, request, building, roof_uri)
   stonehearth.build:grow_roof(session, request, building, roof_uri)
end

function BuildCallHandler:add_portal(session, request, wall, portal_uri, location)
   stonehearth.build:add_portal(session, request, wall, portal_uri, location)
end

return BuildCallHandler
