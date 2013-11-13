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

function BuildCallHandler:_unpackage_proxy_data(proxy)
   local entity = radiant.entities.create_entity(proxy.entity)
   radiant.entities.set_faction(entity, self._session.faction)
   if proxy.components then
      for name, data in pairs(proxy.components) do
         local component = entity:add_component(name)
         assert(component.extend)
         component:extend(data)
      end
   end
   if proxy.children then
      for _, child_proxy in ipairs(proxy.children) do
         local child_entity = self:_unpackage_proxy_data(child_proxy)
         entity:add_component('entity_container'):add_child(child_entity)
      end
   end
   return entity
end

function BuildCallHandler:build_structures(session, request, proxy)
   local root = radiant.entities.get_root_entity()
   local city_plan = root:add_component('stonehearth:city_plan')
   
   -- unpack the structures...
   self._session = session -- xxx: shouldn't call handlers be constructed with the session?
   local entity = self:_unpackage_proxy_data(proxy)
   city_plan:add_blueprint(entity)
   
   return { success = true }
end

return BuildCallHandler
