local Point3 = _radiant.csg.Point3
local BuildCallHandler = class()
local BuildEditor = require 'services.server.build.build_editor'

local build_editor
function BuildCallHandler:get_build_editor(session, request)
   if not build_editor then
      build_editor = BuildEditor()
   end
   return { build_editor = build_editor:get_model() }
end

function BuildCallHandler:build_structures(session, request, changes)
   return stonehearth.build:build_structures(session, changes)
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

return BuildCallHandler
