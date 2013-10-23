local Point3 = _radiant.csg.Point3
local BuildCallHandler = class()
local BuildEditor = require 'services.build.build_editor'

local build_editor
function BuildCallHandler:get_build_editor(session, request)
   if not build_editor then
      build_editor = BuildEditor()
   end
   return { build_editor = build_editor:get_model() }
end

function BuildCallHandler:build_structures(session, request, proxies)
   for _, proxy in ipairs(proxies) do
      local entity = radiant.entities.create_entity(proxy.entity)
      for name, data in pairs(proxy.components) do
         local component = entity:add_component(name)
         component:extend(data)
      end
   end
end

return BuildCallHandler
