local build_util = require 'lib.build_util'

local BuildingClientComponent = class()

function BuildingClientComponent:initialize(entity, json)
   self._entity = entity
end

function BuildingClientComponent:destroy()
end

function BuildingClientComponent:load_from_template(template, options, entity_map)
   self._sv.envelope_entity = build_util.unpack_entity(template.envelope_entity, entity_map)

   self._sv.structures = {}
   for structure_type, ids in pairs(template.structures) do
      local structures = {}
      self._sv.structures[structure_type] = structures
      for _, id in ipairs(ids) do
         local entity = entity_map[id]
         assert(entity)

         structures[entity:get_id()] = {
            entity = entity
         }
      end
   end
   self.__saved_variables:mark_changed()
end

function BuildingClientComponent:get_all_structures()
   return self._sv.structures
end

function BuildingClientComponent:get_building_footprint()
   return self._sv.envelope_entity:get_component('region_collision_shape')
                                       :get_region()
end


return BuildingClientComponent
