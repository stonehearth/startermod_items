local build_util = require 'lib.build_util'
local constants = require('constants').construction

local Floor = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local TraceCategories = _radiant.dm.TraceCategories

-- called to initialize the component on creation and loading.
--
function Floor:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity
   self._sv.category = constants.floor_category.FLOOR
   if not self._sv.connected_to then
      self._sv.connected_to = {}
   end
end

function Floor:destroy()
   if self._mms_trace then
      self._mms_trace:destroy()
      self._mms_trace = nil
   end
end

function Floor:get_region()
   return self._entity:get_component('destination')
                           :get_region()
end

function Floor:layout()
   -- nothing to do...
end

function Floor:get_connected()
   return self._sv.connected_to
end

function Floor:connect_to(blueprint)
   self._sv.connected_to[blueprint:get_id()] = blueprint
   self.__saved_variables:mark_changed()
end

function Floor:set_category(category)
   self._sv.category = category

   if category == constants.floor_category.ROAD and not self._mms_trace then

      local fab = self._entity:get_component('stonehearth:construction_data'):get_fabricator_entity()
      local fc = fab:get_component('stonehearth:fabricator')
      local move_mod = fc:get_project():get_component('movement_modifier_shape')
      move_mod:set_region(_radiant.sim.alloc_region3())

      self._mms_trace = fc:get_project():get_component('region_collision_shape'):trace_region('movement modifier', TraceCategories.SYNC_TRACE)
         :on_changed(function(region)
               move_mod:get_region():modify(function(mod_region)
                     mod_region:copy_region(region:inflated(Point3(0, 1, 0)))
                     mod_region:optimize_by_merge('road movement modifier shape')
                  end)
            end)
         :push_object_state()
   end
end

function Floor:get_category()
   return self._sv.category
end

function Floor:clone_from(entity)
   if entity then
      local other_floor_region = entity:get_component('destination'):get_region():get()

      self._entity:get_component('destination')
                     :get_region()
                        :modify(function(r)
                           r:copy_region(other_floor_region)
                        end)
      end
   return self
end

function Floor:save_to_template()
   return {
      connected_to = build_util.pack_entity_table(self._sv.connected_to),
   }
end

function Floor:load_from_template(data, options, entity_map)
   self._sv.connected_to = build_util.unpack_entity_table(data.connected_to, entity_map)
end

function Floor:rotate_structure(degrees)
   build_util.rotate_structure(self._entity, degrees)
end

return Floor
