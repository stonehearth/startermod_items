local build_util = require 'lib.build_util'

local NoConstructionZoneClientComponent = class()

function NoConstructionZoneClientComponent:initialize(entity, json)
   self._entity = entity
end

function NoConstructionZoneClientComponent:destroy()
   self._dirty = false
end


function NoConstructionZoneClientComponent:trace(reason, category)
   if category then
      return self.__saved_variables:trace(reason, category)
   end
   return self.__saved_variables:trace(reason)
end

function NoConstructionZoneClientComponent:get_overlapping()
   return self._sv.overlapping or {}
end

function NoConstructionZoneClientComponent:load_from_template(template, options, entity_map)
   local region = radiant.alloc_region3()
   region:modify(function(cursor)
         cursor:load(template.shape)
      end)
   self._entity:add_component('region_collision_shape')
                  :set_region(region)
end


return NoConstructionZoneClientComponent

